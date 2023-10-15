/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <Configurations/Coordinator/OptimizerConfiguration.hpp>
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/WatermarkAssignerLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Windowing/WindowLogicalOperatorNode.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QueryPlacement/BasePlacementStrategy.hpp>
#include <Optimizer/QueryRewrite/NemoWindowPinningRule.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/WindowActions/CompleteAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/SliceAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowAggregations/WindowAggregationDescriptor.hpp>
#include <iterator>
#include <vector>

namespace x::Optimizer {

NemoWindowPinningRule::NemoWindowPinningRule(Configurations::OptimizerConfiguration configuration, TopologyPtr topology)
    : DistributedWindowRule(configuration),
      performDistributedWindowOptimization(configuration.performDistributedWindowOptimization),
      windowDistributionChildrenThreshold(configuration.distributedWindowChildThreshold),
      windowDistributionCombinerThreshold(configuration.distributedWindowCombinerThreshold), topology(topology),
      enableNemoPlacement(configuration.enableNemoPlacement) {
    if (performDistributedWindowOptimization) {
        x_DEBUG("Create NemoWindowPinningRule with distributedWindowChildThreshold: {} distributedWindowCombinerThreshold: {}",
                  windowDistributionChildrenThreshold,
                  windowDistributionCombinerThreshold);
    } else {
        x_DEBUG("Disable NemoWindowPinningRule");
    }
};

NemoWindowPinningRulePtr NemoWindowPinningRule::create(Configurations::OptimizerConfiguration configuration,
                                                       TopologyPtr topology) {
    x_ASSERT(topology != nullptr, "DistributedWindowRule: Topology is null");
    return std::make_shared<NemoWindowPinningRule>(NemoWindowPinningRule(configuration, topology));
}

QueryPlanPtr NemoWindowPinningRule::apply(QueryPlanPtr queryPlan) {
    x_DEBUG("NemoWindowPinningRule: Apply NemoWindowPinningRule.");
    x_DEBUG("NemoWindowPinningRule::apply: plan before replace\n{}", queryPlan->toString());
    if (!performDistributedWindowOptimization) {
        return queryPlan;
    }
    auto windowOps = queryPlan->getOperatorByType<WindowLogicalOperatorNode>();
    if (!windowOps.empty()) {
        /**
         * @end
         */
        x_DEBUG("NemoWindowPinningRule::apply: found {} window operators", windowOps.size());
        for (auto& windowOp : windowOps) {
            x_DEBUG("NemoWindowPinningRule::apply: window operator {}", windowOp->toString());

            if (windowOp->getChildren().size() >= windowDistributionChildrenThreshold
                && windowOp->getWindowDefinition()->getWindowAggregation().size() == 1) {
                if (enableNemoPlacement) {
                    pinWindowOperators(windowOp, queryPlan);
                } else {
                    createDistributedWindowOperator(windowOp, queryPlan);// should be removed in the future
                }
            } else {
                createCentralWindowOperator(windowOp);
                x_DEBUG("NemoWindowPinningRule::apply: central op\n{}", queryPlan->toString());
            }
        }
    } else {
        x_DEBUG("NemoWindowPinningRule::apply: no window operator in query");
    }
    x_DEBUG("NemoWindowPinningRule::apply: plan after replace\n{}", queryPlan->toString());
    return queryPlan;
}

void NemoWindowPinningRule::createCentralWindowOperator(const WindowOperatorNodePtr& windowOp) {
    x_DEBUG("NemoWindowPinningRule::apply: introduce centralized window operator for window {}", windowOp->toString());
    auto newWindowOp = LogicalOperatorFactory::createCentralWindowSpecializedOperator(windowOp->getWindowDefinition());
    x_DEBUG("NemoWindowPinningRule::apply: newNode={} old node={}", newWindowOp->toString(), windowOp->toString());
    windowOp->replace(newWindowOp);
}

void NemoWindowPinningRule::pinWindowOperators(const WindowOperatorNodePtr& windowOp, const QueryPlanPtr& queryPlan) {
    x_DEBUG("NemoWindowPinningRule::apply: introduce new distributed window operator for window {}", windowOp->toString());
    auto parents = windowOp->getParents();
    auto mergerNodes = getMergerNodes(windowOp, windowDistributionCombinerThreshold);
    windowOp->removeChildren();
    windowOp->removeAllParent();

    for (auto parent : parents) {
        parent->removeChildren();

        for (auto mergerPair : mergerNodes) {
            auto nodeId = mergerPair.first;
            auto newWindowOp = LogicalOperatorFactory::createCentralWindowSpecializedOperator(windowOp->getWindowDefinition());
            newWindowOp->addProperty(x::Optimizer::PINNED_NODE_ID, nodeId);
            x_DEBUG("NemoWindowPinningRule::apply: newNode={} old node={}", newWindowOp->toString(), windowOp->toString());

            auto children = mergerPair.second;
            for (auto source : children) {
                parent->addChild(newWindowOp);
                newWindowOp->addChild(source);
            }
        }
    }
    x_DEBUG("DistributedWindowRule: Plan after\n{}", queryPlan->toString());
}

std::unordered_map<uint64_t, std::vector<WatermarkAssignerLogicalOperatorNodePtr>>
NemoWindowPinningRule::getMergerNodes(OperatorNodePtr operatorNode, uint64_t sharedParentThreshold) {
    std::unordered_map<uint64_t, std::vector<std::pair<TopologyNodePtr, WatermarkAssignerLogicalOperatorNodePtr>>> nodePlacement;
    //iterate over all children of the operator
    for (auto child : operatorNode->getAndFlattenAllChildren(true)) {
        if (child->as_if<OperatorNode>()->hasProperty(x::Optimizer::PINNED_NODE_ID)) {
            auto nodeId = std::any_cast<uint64_t>(child->as_if<OperatorNode>()->getProperty(x::Optimizer::PINNED_NODE_ID));
            TopologyNodePtr node = topology->findNodeWithId(nodeId);
            for (auto& parent : node->getParents()) {
                auto parentId = std::any_cast<uint64_t>(parent->as_if<TopologyNode>()->getId());

                // get the watermark parent
                WatermarkAssignerLogicalOperatorNodePtr watermark;
                for (auto ancestor : child->getAndFlattenAllAncestors()) {
                    if (ancestor->instanceOf<WatermarkAssignerLogicalOperatorNode>()) {
                        watermark = ancestor->as_if<WatermarkAssignerLogicalOperatorNode>();
                        break;
                    }
                }
                x_ASSERT(watermark != nullptr, "DistributedWindowRule: Window source does not contain a watermark");

                auto newPair = std::make_pair(node, watermark);
                //identify shared parent and add to result
                if (nodePlacement.contains(parentId)) {
                    nodePlacement[parentId].emplace_back(newPair);
                } else {
                    nodePlacement[parentId] =
                        std::vector<std::pair<TopologyNodePtr, WatermarkAssignerLogicalOperatorNodePtr>>{newPair};
                }
            }
        }
    }
    std::vector<std::pair<TopologyNodePtr, WatermarkAssignerLogicalOperatorNodePtr>> rootOperators;
    auto rootId = topology->getRoot()->getId();

    //get the root operators
    if (nodePlacement.contains(rootId)) {
        rootOperators = nodePlacement[rootId];
    } else {
        nodePlacement[rootId] = rootOperators;
    }

    // add windows under the threshold to the root
    std::unordered_map<uint64_t, std::vector<WatermarkAssignerLogicalOperatorNodePtr>> output;
    for (auto plcmnt : nodePlacement) {
        if (plcmnt.second.size() <= sharedParentThreshold) {
            // resolve the placement
            for (auto pairs : plcmnt.second) {
                output[pairs.first->getId()] = std::vector<WatermarkAssignerLogicalOperatorNodePtr>{pairs.second};
            }
        } else {
            // add to output
            if (plcmnt.second.size() > 1) {
                auto addedNodes = std::vector<WatermarkAssignerLogicalOperatorNodePtr>{};
                for (auto pairs : plcmnt.second) {
                    addedNodes.emplace_back(pairs.second);
                }
                output[plcmnt.first] = addedNodes;
            } else {
                // place at the root of topology if there is no shared parent
                if (output.contains(rootId)) {
                    output[rootId].emplace_back(plcmnt.second[0].second);
                } else {
                    output[rootId] = std::vector<WatermarkAssignerLogicalOperatorNodePtr>{plcmnt.second[0].second};
                }
            }
        }
    }
    return output;
}

void NemoWindowPinningRule::createDistributedWindowOperator(const WindowOperatorNodePtr& logicalWindowOperator,
                                                            const QueryPlanPtr& queryPlan) {
    // To distribute the window operator we replace the current window operator with 1 WindowComputationOperator (performs the final aggregate)
    // and n SliceCreationOperators.
    // To this end, we have to a the window definitions in the following way:
    // The SliceCreation consumes input and outputs data in the schema: {startTs, endTs, keyField, value}
    // The WindowComputation consumes that schema and outputs data in: {startTs, endTs, keyField, outputAggField}
    // First we prepare the final WindowComputation operator:

    //if window has more than 4 edges, we introduce a combiner

    x_DEBUG("NemoWindowPinningRule::apply: introduce distributed window operator for window {}",
              logicalWindowOperator->toString());
    auto windowDefinition = logicalWindowOperator->getWindowDefinition();
    auto triggerPolicy = windowDefinition->getTriggerPolicy();
    auto triggerActionComplete = Windowing::CompleteAggregationTriggerActionDescriptor::create();
    auto windowType = windowDefinition->getWindowType();
    auto windowAggregation = windowDefinition->getWindowAggregation();
    auto keyField = windowDefinition->getKeys();
    auto allowedLatexs = windowDefinition->getAllowedLatexs();
    // For the final window computation we have to change copy aggregation function and manipulate the fields we want to aggregate.
    auto windowComputationAggregation = windowAggregation[0]->copy();
    //    windowComputationAggregation->on()->as<FieldAccessExpressionNode>()->setFieldName("value");

    Windowing::LogicalWindowDefinitionPtr windowDef;
    if (logicalWindowOperator->getWindowDefinition()->isKeyed()) {
        windowDef = Windowing::LogicalWindowDefinition::create(keyField,
                                                               {windowComputationAggregation},
                                                               windowType,
                                                               Windowing::DistributionCharacteristic::createCombiningWindowType(),
                                                               triggerPolicy,
                                                               triggerActionComplete,
                                                               allowedLatexs);

    } else {
        windowDef = Windowing::LogicalWindowDefinition::create({windowComputationAggregation},
                                                               windowType,
                                                               Windowing::DistributionCharacteristic::createCombiningWindowType(),
                                                               triggerPolicy,
                                                               triggerActionComplete,
                                                               allowedLatexs);
    }
    x_DEBUG("NemoWindowPinningRule::apply: created logical window definition for computation operator{}",
              windowDef->toString());

    auto windowComputationOperator = LogicalOperatorFactory::createWindowComputationSpecializedOperator(windowDef);

    //replace logical window op with window computation operator
    x_DEBUG("NemoWindowPinningRule::apply: newNode={} old node={}",
              windowComputationOperator->toString(),
              logicalWindowOperator->toString());
    if (!logicalWindowOperator->replace(windowComputationOperator)) {
        x_FATAL_ERROR("NemoWindowPinningRule:: replacement of window operator failed.");
    }

    auto windowChildren = windowComputationOperator->getChildren();

    auto assignerOp = queryPlan->getOperatorByType<WatermarkAssignerLogicalOperatorNode>();
    UnaryOperatorNodePtr finalComputationAssigner = windowComputationOperator;
    x_ASSERT(assignerOp.size() > 1, "at least one assigner has to be there");

    //add merger
    UnaryOperatorNodePtr mergerAssigner;
    if (finalComputationAssigner->getChildren().size() >= windowDistributionCombinerThreshold) {
        auto sliceCombinerWindowAggregation = windowAggregation[0]->copy();

        if (logicalWindowOperator->getWindowDefinition()->isKeyed()) {
            windowDef =
                Windowing::LogicalWindowDefinition::create(keyField,
                                                           {sliceCombinerWindowAggregation},
                                                           windowType,
                                                           Windowing::DistributionCharacteristic::createMergingWindowType(),
                                                           triggerPolicy,
                                                           Windowing::SliceAggregationTriggerActionDescriptor::create(),
                                                           allowedLatexs);

        } else {
            windowDef =
                Windowing::LogicalWindowDefinition::create({sliceCombinerWindowAggregation},
                                                           windowType,
                                                           Windowing::DistributionCharacteristic::createMergingWindowType(),
                                                           triggerPolicy,
                                                           Windowing::SliceAggregationTriggerActionDescriptor::create(),
                                                           allowedLatexs);
        }
        x_DEBUG("NemoWindowPinningRule::apply: created logical window definition for slice merger operator {}",
                  windowDef->toString());
        auto sliceOp = LogicalOperatorFactory::createSliceMergingSpecializedOperator(windowDef);
        finalComputationAssigner->insertBetweenThisAndChildNodes(sliceOp);

        mergerAssigner = sliceOp;
        windowChildren = mergerAssigner->getChildren();
    }

    //adding slicer
    for (auto& child : windowChildren) {
        x_DEBUG("NemoWindowPinningRule::apply: process child {}", child->toString());

        // For the SliceCreation operator we have to change copy aggregation function and manipulate the fields we want to aggregate.
        auto sliceCreationWindowAggregation = windowAggregation[0]->copy();
        auto triggerActionSlicing = Windowing::SliceAggregationTriggerActionDescriptor::create();

        if (logicalWindowOperator->getWindowDefinition()->isKeyed()) {
            windowDef =
                Windowing::LogicalWindowDefinition::create({keyField},
                                                           {sliceCreationWindowAggregation},
                                                           windowType,
                                                           Windowing::DistributionCharacteristic::createSlicingWindowType(),
                                                           triggerPolicy,
                                                           triggerActionSlicing,
                                                           allowedLatexs);
        } else {
            windowDef =
                Windowing::LogicalWindowDefinition::create({sliceCreationWindowAggregation},
                                                           windowType,
                                                           Windowing::DistributionCharacteristic::createSlicingWindowType(),
                                                           triggerPolicy,
                                                           triggerActionSlicing,
                                                           allowedLatexs);
        }
        x_DEBUG("NemoWindowPinningRule::apply: created logical window definition for slice operator {}", windowDef->toString());
        auto sliceOp = LogicalOperatorFactory::createSliceCreationSpecializedOperator(windowDef);
        child->insertBetweenThisAndParentNodes(sliceOp);
    }
}

}// namespace x::Optimizer
