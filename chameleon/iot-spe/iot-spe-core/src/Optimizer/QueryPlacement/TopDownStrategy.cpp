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

#include <API/Query.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <Exceptions/QueryPlacementException.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QueryPlacement/TopDownStrategy.hpp>
#include <Plans/Global/Execution/ExecutionNode.hpp>
#include <Plans/Global/Execution/GlobalExecutionPlan.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Plans/Utils/QueryPlanIterator.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>

#include <utility>

namespace x::Optimizer {

BasePlacementStrategyPtr TopDownStrategy::create(GlobalExecutionPlanPtr globalExecutionPlan,
                                                 TopologyPtr topology,
                                                 TypeInferencePhasePtr typeInferencePhase) {
    return std::make_unique<TopDownStrategy>(
        TopDownStrategy(std::move(globalExecutionPlan), std::move(topology), std::move(typeInferencePhase)));
}

TopDownStrategy::TopDownStrategy(GlobalExecutionPlanPtr globalExecutionPlan,
                                 TopologyPtr topology,
                                 TypeInferencePhasePtr typeInferencePhase)
    : BasePlacementStrategy(std::move(globalExecutionPlan), std::move(topology), std::move(typeInferencePhase)) {}

bool TopDownStrategy::updateGlobalExecutionPlan(QueryId queryId,
                                                FaultToleranceType faultToleranceType,
                                                LineageType lineageType,
                                                const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                                const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) {
    try {
        x_DEBUG("Perform placement of the pinned and all their downstream operators.");
        // 1. Find the path where operators need to be placed
        performPathSelection(pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 2. Place operators on the selected path
        pinOperators(queryId, pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 3. Place pinned operators
        placePinnedOperators(queryId, pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 4. add network source and sink operators
        addNetworkSourceAndSinkOperators(queryId, pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 5. Perform type inference on all updated query plans
        return runTypeInferencePhase(queryId, faultToleranceType, lineageType);
    } catch (std::exception& ex) {
        throw Exceptions::QueryPlacementException(queryId, ex.what());
    }
}

void TopDownStrategy::pinOperators(QueryId queryId,
                                   const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                   const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) {

    x_TRACE("TopDownStrategy: Place all sink operators.");
    for (const auto& pinnedDownStreamOperator : pinnedDownStreamOperators) {
        x_TRACE("TopDownStrategy: Get the topology node for the sink operator.");
        auto nodeId = std::any_cast<uint64_t>(pinnedDownStreamOperator->getProperty(PINNED_NODE_ID));
        TopologyNodePtr candidateTopologyNode = getTopologyNode(nodeId);

        // 1. If pinned down stream operator was already placed then place all its upstream operators
        if (pinnedDownStreamOperator->getOperatorState() == OperatorState::PLACED) {
            //Fetch the execution node storing the operator
            operatorToExecutionNodeMap[pinnedDownStreamOperator->getId()] = globalExecutionPlan->getExecutionNodeByNodeId(nodeId);
            //Place all downstream nodes
            for (auto& upStreamOperator : pinnedDownStreamOperator->getChildren()) {
                identifyPinningLocation(queryId,
                                        upStreamOperator->as<LogicalOperatorNode>(),
                                        candidateTopologyNode,
                                        pinnedUpStreamOperators);
            }
        } else {// 2. If pinned operator is not placed then start by placing the operator
            if (candidateTopologyNode->getAvailableResources() == 0
                && !operatorToExecutionNodeMap.contains(pinnedDownStreamOperator->getId())) {
                x_ERROR("BottomUpStrategy: Unable to find resources on the physical node for placement of source operator");
                throw Exceptions::RuntimeException(
                    "BottomUpStrategy: Unable to find resources on the physical node for placement of source operator");
            }
            identifyPinningLocation(queryId, pinnedDownStreamOperator, candidateTopologyNode, pinnedUpStreamOperators);
        }
    }
    x_DEBUG("TopDownStrategy: Finished placing query operators into the global execution plan");
}

void TopDownStrategy::identifyPinningLocation(QueryId queryId,
                                              const LogicalOperatorNodePtr& logicalOperator,
                                              TopologyNodePtr candidateTopologyNode,
                                              const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators) {

    if (logicalOperator->getOperatorState() == OperatorState::PLACED) {
        x_DEBUG("Operator is already placed and thus skipping placement of this and its down stream operators.");
        auto nodeId = std::any_cast<uint64_t>(logicalOperator->getProperty(PINNED_NODE_ID));
        operatorToExecutionNodeMap[logicalOperator->getId()] = globalExecutionPlan->getExecutionNodeByNodeId(nodeId);
        return;
    }

    if (!operatorToExecutionNodeMap.contains(logicalOperator->getId())) {
        x_DEBUG("TopDownStrategy: Place {}", logicalOperator->toString());
        if ((logicalOperator->hasMultipleChildrenOrParents() || logicalOperator->instanceOf<SourceLogicalOperatorNode>())
            && !logicalOperator->instanceOf<SinkLogicalOperatorNode>()) {

            x_TRACE("TopDownStrategy: Received an NAry operator for placement.");
            x_TRACE("TopDownStrategy: Get the topology nodes where parent operators are placed.");
            std::vector<TopologyNodePtr> parentTopologyNodes = getTopologyNodesForDownStreamOperators(logicalOperator);
            if (parentTopologyNodes.empty()) {
                x_WARNING("TopDownStrategy: No topology node found where parent operators are placed.");
                return;
            }

            x_TRACE("TopDownStrategy: Get the topology nodes where children source operators are to be placed.");
            std::vector<TopologyNodePtr> childNodes = getTopologyNodesForUpStreamOperators(logicalOperator);

            x_TRACE("TopDownStrategy: Find a node reachable from all child and parent topology nodes.");
            candidateTopologyNode = topology->findCommonNodeBetween(childNodes, parentTopologyNodes);

            if (!candidateTopologyNode) {
                x_ERROR("TopDownStrategy: Unable to find the candidate topology node for placing Nary operator {}",
                          logicalOperator->toString());
                throw Exceptions::RuntimeException(
                    "TopDownStrategy: Unable to find the candidate topology node for placing Nary operator "
                    + logicalOperator->toString());
            }

            if (logicalOperator->instanceOf<SourceLogicalOperatorNode>()) {
                x_DEBUG("TopDownStrategy: Received Source operator for placement.");
                auto nodeId = std::any_cast<uint64_t>(logicalOperator->getProperty(PINNED_NODE_ID));
                auto pinnedSourceOperatorLocation = getTopologyNode(nodeId);
                if (pinnedSourceOperatorLocation->getId() == candidateTopologyNode->getId()
                    || pinnedSourceOperatorLocation->containAsParent(candidateTopologyNode)) {
                    candidateTopologyNode = pinnedSourceOperatorLocation;
                } else {
                    x_ERROR("TopDownStrategy: Unexpected behavior. Could not find Topology node where source operator is to be "
                              "placed.");
                    throw Exceptions::RuntimeException(
                        "TopDownStrategy: Unexpected behavior. Could not find Topology node where source operator is "
                        "to be placed.");
                }

                if (candidateTopologyNode->getAvailableResources() == 0) {
                    x_ERROR("TopDownStrategy: Topology node where source operator is to be placed has no capacity.");
                    throw Exceptions::RuntimeException(
                        "TopDownStrategy: Topology node where source operator is to be placed has no capacity.");
                }
            }
        }

        if (candidateTopologyNode->getAvailableResources() == 0) {
            x_DEBUG("TopDownStrategy: Find the next Topology node where operator can be placed");
            std::vector<TopologyNodePtr> childNodes = getTopologyNodesForUpStreamOperators(logicalOperator);
            x_TRACE("TopDownStrategy: Find a node reachable from all child and parent topology nodes.");
            //FIXME: we are considering only one root topology node currently
            auto candidateTopologyNodes = topology->findNodesBetween(childNodes, {candidateTopologyNode});
            for (const auto& topologyNodes : candidateTopologyNodes) {
                if (topologyNodes && topologyNodes->getAvailableResources() > 0) {
                    candidateTopologyNode = topologyNodes;
                    x_DEBUG("TopDownStrategy: Found x node for placing the operators with id : {}",
                              candidateTopologyNode->getId());
                    break;
                }
            }
        }

        if (!candidateTopologyNode || candidateTopologyNode->getAvailableResources() == 0) {
            x_ERROR("TopDownStrategy: No node available for further placement of operators");
            throw Exceptions::RuntimeException("TopDownStrategy: No node available for further placement of operators");
        }

        //Pin the operator to the candidate node and mark as placed
        logicalOperator->addProperty(PINNED_NODE_ID, candidateTopologyNode->getId());

    } else {
        candidateTopologyNode = operatorToExecutionNodeMap[logicalOperator->getId()]->getTopologyNode();
    }

    auto isOperatorAPinnedUpStreamOperator = std::find_if(pinnedUpStreamOperators.begin(),
                                                          pinnedUpStreamOperators.end(),
                                                          [logicalOperator](const OperatorNodePtr& pinnedUpStreamOperator) {
                                                              return pinnedUpStreamOperator->getId() == logicalOperator->getId();
                                                          });

    if (isOperatorAPinnedUpStreamOperator != pinnedUpStreamOperators.end()) {
        x_DEBUG("BottomUpStrategy: Found pinned downstream operator. Skipping placement of further operators.");
        return;
    }

    x_TRACE("TopDownStrategy: Place the children operators.");
    for (const auto& upstreamOperator : logicalOperator->getChildren()) {
        identifyPinningLocation(queryId,
                                upstreamOperator->as<LogicalOperatorNode>(),
                                candidateTopologyNode,
                                pinnedUpStreamOperators);
    }
}

std::vector<TopologyNodePtr>
TopDownStrategy::getTopologyNodesForDownStreamOperators(const LogicalOperatorNodePtr& candidateOperator) {

    std::vector<TopologyNodePtr> parentTopologyNodes;
    x_DEBUG("TopDownStrategy: Get topology nodes with parent operators");
    std::vector<NodePtr> downstreamOperators = candidateOperator->getParents();
    //iterate over parent operators and get the physical location where operator is placed
    for (auto& downstreamOperator : downstreamOperators) {
        if (!downstreamOperator->as_if<OperatorNode>()->hasProperty(PINNED_NODE_ID)) {
            x_WARNING("TopDownStrategy: unable to find topology for downstreamOperator operator.");
            return {};
        }

        TopologyNodePtr parentTopologyNode =
            topologyMap[std::any_cast<uint64_t>(downstreamOperator->as_if<OperatorNode>()->getProperty(PINNED_NODE_ID))];
        parentTopologyNodes.push_back(parentTopologyNode);
    }
    x_DEBUG("TopDownStrategy: returning list of topology nodes where parent operators are placed");
    return parentTopologyNodes;
}

std::vector<TopologyNodePtr>
TopDownStrategy::getTopologyNodesForUpStreamOperators(const LogicalOperatorNodePtr& candidateOperator) {
    std::vector<TopologyNodePtr> upStreamTopologyNodes;

    x_TRACE(
        "TopDownStrategy::getTopologyNodesForUpStreamOperators: Get the pinned or closest placed upStreamOperators nodes for "
        "the the input operator.");
    std::vector<NodePtr> upStreamOperators = {candidateOperator};

    while (!upStreamOperators.empty()) {
        auto upStreamOperator = upStreamOperators.back()->as<OperatorNode>();
        upStreamOperators.pop_back();
        if (upStreamOperator->hasProperty(PINNED_NODE_ID)) {
            auto nodeId = std::any_cast<uint64_t>(upStreamOperator->getProperty(PINNED_NODE_ID));
            auto pinnedTopologyNode = getTopologyNode(nodeId);
            upStreamTopologyNodes.emplace_back(pinnedTopologyNode);
            continue;
        }
        upStreamOperators.insert(upStreamOperators.end(),
                                 upStreamOperator->getChildren().begin(),
                                 upStreamOperator->getChildren().end());
    }
    if (upStreamTopologyNodes.empty()) {
        x_ERROR("TopDownStrategy::getTopologyNodesForUpStreamOperators: Unable to find the upStreamOperators operators to the "
                  "candidate operator");
        throw Exceptions::RuntimeException(
            "TopDownStrategy::getTopologyNodesForUpStreamOperators: Unable to find the upStreamOperators operators to the "
            "candidate operator");
    }
    return upStreamTopologyNodes;
}
}// namespace x::Optimizer
