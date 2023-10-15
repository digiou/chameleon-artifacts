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
#include <Optimizer/QueryRewrite/DistributedWindowRule.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/WindowActions/CompleteAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/SliceAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowAggregations/WindowAggregationDescriptor.hpp>
namespace x::Optimizer {

DistributedWindowRule::DistributedWindowRule(Configurations::OptimizerConfiguration configuration)
    : performDistributedWindowOptimization(configuration.performDistributedWindowOptimization),
      windowDistributionChildrenThreshold(configuration.distributedWindowChildThreshold),
      windowDistributionCombinerThreshold(configuration.distributedWindowCombinerThreshold) {
    if (performDistributedWindowOptimization) {
        x_DEBUG("Create DistributedWindowRule with distributedWindowChildThreshold: {} distributedWindowCombinerThreshold: {}",
                  windowDistributionChildrenThreshold,
                  windowDistributionCombinerThreshold);
    } else {
        x_DEBUG("Disable DistributedWindowRule");
    }
};

DistributeWindowRulePtr DistributedWindowRule::create(Configurations::OptimizerConfiguration configuration) {
    return std::make_shared<DistributedWindowRule>(DistributedWindowRule(configuration));
}

QueryPlanPtr DistributedWindowRule::apply(QueryPlanPtr queryPlan) {
    x_DEBUG("DistributedWindowRule: Apply DistributedWindowRule.");
    x_DEBUG("DistributedWindowRule::apply: plan before replace {}", queryPlan->toString());

    auto windowOps = queryPlan->getOperatorByType<WindowLogicalOperatorNode>();
    x_DEBUG("DistributedWindowRule::apply: found {} window operators", windowOps.size());
    for (auto& windowOp : windowOps) {
        x_DEBUG("DistributedWindowRule::apply: window operator {}", windowOp->toString());
        if (performDistributedWindowOptimization && windowOp->getChildren().size() >= windowDistributionChildrenThreshold
            && windowOp->getWindowDefinition()->getWindowAggregation().size() == 1) {
            createDistributedWindowOperator(windowOp, queryPlan);
        } else {
            createCentralWindowOperator(windowOp);
            x_DEBUG("DistributedWindowRule::apply: central op\n{}", queryPlan->toString());
        }
    }

    x_DEBUG("DistributedWindowRule::apply: plan after replace {}", queryPlan->toString());
    return queryPlan;
}

void DistributedWindowRule::createCentralWindowOperator(const WindowOperatorNodePtr& windowOp) {
    x_DEBUG("DistributedWindowRule::apply: introduce centralized window operator for window {}", windowOp->toString());
    auto newWindowOp = LogicalOperatorFactory::createCentralWindowSpecializedOperator(windowOp->getWindowDefinition());
    newWindowOp->setInputSchema(windowOp->getInputSchema());
    newWindowOp->setOutputSchema(windowOp->getOutputSchema());
    x_DEBUG("DistributedWindowRule::apply: newNode={} old node={}", newWindowOp->toString(), windowOp->toString());
    windowOp->replace(newWindowOp);
}

void DistributedWindowRule::createDistributedWindowOperator(const WindowOperatorNodePtr& logicalWindowOperator,
                                                            const QueryPlanPtr& queryPlan) {
    // To distribute the window operator we replace the current window operator with 1 WindowComputationOperator (performs the final aggregate)
    // and n SliceCreationOperators.
    // To this end, we have to a the window definitions in the following way:
    // The SliceCreation consumes input and outputs data in the schema: {startTs, endTs, keyField, value}
    // The WindowComputation consumes that schema and outputs data in: {startTs, endTs, keyField, outputAggField}
    // First we prepare the final WindowComputation operator:

    //if window has more than 4 edges, we introduce a combiner

    x_DEBUG("DistributedWindowRule::apply: introduce distributed window operator for window {}",
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
    x_DEBUG("DistributedWindowRule::apply: created logical window definition for computation operator{}",
              windowDef->toString());

    auto windowComputationOperator = LogicalOperatorFactory::createWindowComputationSpecializedOperator(windowDef);

    //replace logical window op with window computation operator
    x_DEBUG("DistributedWindowRule::apply: newNode={} old node={}",
              windowComputationOperator->toString(),
              logicalWindowOperator->toString());
    if (!logicalWindowOperator->replace(windowComputationOperator)) {
        x_FATAL_ERROR("DistributedWindowRule:: replacement of window operator failed.");
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
        x_DEBUG("DistributedWindowRule::apply: created logical window definition for slice merger operator {}",
                  windowDef->toString());
        auto sliceOp = LogicalOperatorFactory::createSliceMergingSpecializedOperator(windowDef);
        finalComputationAssigner->insertBetweenThisAndChildNodes(sliceOp);

        mergerAssigner = sliceOp;
        windowChildren = mergerAssigner->getChildren();
    }

    //adding slicer
    for (auto& child : windowChildren) {
        x_DEBUG("DistributedWindowRule::apply: process child {}", child->toString());

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
        x_DEBUG("DistributedWindowRule::apply: created logical window definition for slice operator {}", windowDef->toString());
        auto sliceOp = LogicalOperatorFactory::createSliceCreationSpecializedOperator(windowDef);
        child->insertBetweenThisAndParentNodes(sliceOp);
    }
}

}// namespace x::Optimizer
