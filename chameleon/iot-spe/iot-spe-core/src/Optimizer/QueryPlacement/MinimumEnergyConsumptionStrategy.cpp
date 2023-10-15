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
#include <Catalogs/SourceCatalog.hpp>
#include <Operators/LogicalOperators/LogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/LogicalSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Operators/Operator.hpp>
#include <Optimizer/ExecutionNode.hpp>
#include <Optimizer/xExecutionPlan.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QueryPlacement/MinimumEnergyConsumptionStrategy.hpp>
#include <Optimizer/Utils/PathFinder.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/CodeGenerator/TranslateToLegacyExpression.hpp>
#include <Topology/xTopologyPlan.hpp>
#include <Util/Logger/Logger.hpp>

namespace x {

xExecutionPlanPtr MinimumEnergyConsumptionStrategy::initializeExecutionPlan(QueryPlanPtr queryPlan,
                                                                              xTopologyPlanPtr xTopologyPlan,
                                                                              Catalogs::Source::SourceCatalogPtr sourceCatalog) {
    this->xTopologyPlan = xTopologyPlan;
    const SourceLogicalOperatorNodePtr sourceOperator = queryPlan->getSourceOperators()[0];

    // FIXME: current implementation assumes that we have only one source and therefore only one source operator.
    const string sourceName = queryPlan->getSourceName();

    if (!sourceOperator) {
        x_THROW_RUNTIME_ERROR("MinimumEnergyConsumption: Unable to find the source operator.");
    }

    const vector<xTopologyEntryPtr> sourceNodes = sourceCatalog->getSourceNodesForLogicalSource(sourceName);

    if (sourceNodes.empty()) {
        x_THROW_RUNTIME_ERROR("MinimumEnergyConsumption: Unable to find the target source: " + sourceName);
    }

    xExecutionPlanPtr xExecutionPlanPtr = std::make_shared<xExecutionPlan>();
    const xTopologyGraphPtr xTopologyGraphPtr = xTopologyPlan->getxTopologyGraph();

    x_INFO("MinimumEnergyConsumption: Placing operators on the x topology.");
    placeOperators(xExecutionPlanPtr, xTopologyGraphPtr, sourceOperator, sourceNodes);

    xTopologyEntryPtr rootNode = xTopologyGraphPtr->getRoot();

    x_DEBUG("MinimumEnergyConsumption: Find the path used for performing the placement based on the strategy type");
    vector<xTopologyEntryPtr> candidateNodes = getCandidateNodesForFwdOperatorPlacement(sourceNodes, rootNode);

    x_INFO("MinimumEnergyConsumption: Adding forward operators.");
    addSystemGeneratedOperators(candidateNodes, xExecutionPlanPtr);

    x_INFO("MinimumEnergyConsumption: Generating complete execution Graph.");
    fillExecutionGraphWithTopologyInformation(xExecutionPlanPtr, xTopologyPlan);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    SchemaPtr schema = sourceOperator->getSourceDescriptor()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, xExecutionPlanPtr);

    return xExecutionPlanPtr;
}

vector<xTopologyEntryPtr>
MinimumEnergyConsumptionStrategy::getCandidateNodesForFwdOperatorPlacement(const vector<xTopologyEntryPtr>& sourceNodes,
                                                                           const x::xTopologyEntryPtr rootNode) const {
    auto pathMap = pathFinder->findUniquePathBetween(sourceNodes, rootNode);
    vector<xTopologyEntryPtr> candidateNodes;
    for (auto [key, value] : pathMap) {
        candidateNodes.insert(candidateNodes.end(), value.begin(), value.end());
    }
    return candidateNodes;
}

void MinimumEnergyConsumptionStrategy::placeOperators(xExecutionPlanPtr executionPlanPtr,
                                                      xTopologyGraphPtr xTopologyGraphPtr,
                                                      LogicalOperatorNodePtr sourceOperator,
                                                      vector<xTopologyEntryPtr> sourceNodes) {

    TranslateToLegacyPlanPhasePtr translator = TranslateToLegacyPlanPhase::create();
    const xTopologyEntryPtr sinkNode = xTopologyGraphPtr->getRoot();

    x_INFO("MinimumEnergyConsumption: preparing common path between sources");
    vector<xTopologyEntryPtr> commonPath;
    auto pathMap = pathFinder->findUniquePathBetween(sourceNodes, sinkNode);

    //Prepare list of ordered common nodes
    vector<vector<xTopologyEntryPtr>> listOfPaths;
    transform(pathMap.begin(), pathMap.end(), back_inserter(listOfPaths), [](const auto pair) {
        return pair.second;
    });

    for (uint64_t i = 0; i < listOfPaths.size(); i++) {
        vector<xTopologyEntryPtr> path_i = listOfPaths[i];

        for (xTopologyEntryPtr node_i : path_i) {
            bool nodeOccursInAllPaths = false;
            for (uint64_t j = i; j < listOfPaths.size(); j++) {
                if (i == j) {
                    continue;
                }

                vector<xTopologyEntryPtr> path_j = listOfPaths[j];
                const auto itr = find_if(path_j.begin(), path_j.end(), [node_i](xTopologyEntryPtr node_j) {
                    return node_i->getId() == node_j->getId();
                });

                if (itr != path_j.end()) {
                    nodeOccursInAllPaths = true;
                } else {
                    nodeOccursInAllPaths = false;
                    break;
                }
            }

            if (nodeOccursInAllPaths) {
                commonPath.push_back(node_i);
            }
        }
    }

    x_INFO("MinimumEnergyConsumption: Sort all paths in increasing order of compute resources");
    //Sort all the paths with increased aggregated compute capacity
    vector<std::pair<uint64_t, int>> computeCostList;

    //Calculate total compute cost for each path
    for (uint64_t i = 0; i < listOfPaths.size(); i++) {
        vector<xTopologyEntryPtr> path = listOfPaths[i];
        uint64_t totalComputeForPath = 0;
        for (xTopologyEntryPtr node : path) {
            totalComputeForPath = totalComputeForPath + node->getCpuCapacity();
        }
        computeCostList.push_back(make_pair(totalComputeForPath, i));
    }

    sort(computeCostList.begin(), computeCostList.end());

    vector<vector<xTopologyEntryPtr>> sortedListOfPaths;
    for (auto pair : computeCostList) {
        sortedListOfPaths.push_back(listOfPaths[pair.second]);
    }

    uint64_t lastPlacedOperatorId;
    x_INFO("MinimumEnergyConsumption: place all non blocking operators starting from source first");
    for (auto path : sortedListOfPaths) {
        LogicalOperatorNodePtr targetOperator = sourceOperator;
        x_DEBUG("MinimumEnergyConsumption: Transforming New Operator into legacy operator");
        OperatorPtr legacyOperator = translator->transform(targetOperator);
        while (!operatorIsBlocking[legacyOperator->getOperatorType()] && targetOperator->instanceOf<SinkLogicalOperatorNode>()) {

            if (targetOperator->instanceOf<SourceLogicalOperatorNode>()) {
                x_INFO("MinimumEnergyConsumption: find if the target non blocking operator already scheduled on common path");
                bool foundOperator = false;
                for (auto commonNode : commonPath) {
                    const ExecutionNodePtr executionNode = executionPlanPtr->getExecutionNode(commonNode->getId());
                    if (executionNode) {
                        vector<uint64_t> scheduledOperators = executionNode->getChildOperatorIds();
                        const auto foundItr =
                            std::find_if(scheduledOperators.begin(), scheduledOperators.end(), [targetOperator](uint64_t optrId) {
                                return optrId == targetOperator->getId();
                            });
                        if (foundItr != scheduledOperators.end()) {
                            foundOperator = true;
                            break;
                        }
                    }
                }

                if (foundOperator) {
                    x_INFO("MinimumEnergyConsumption: found target operator already scheduled.");
                    x_INFO("MinimumEnergyConsumption: Skipping rest of the placement for current physical sensor.");
                    break;
                }
            }

            if (lastPlacedOperatorId < targetOperator->getId()) {
                lastPlacedOperatorId = targetOperator->getId();
            }

            xTopologyEntryPtr node = nullptr;
            for (xTopologyEntryPtr pathNode : path) {
                if (pathNode->getRemainingCpuCapacity() > 0) {
                    node = pathNode;
                    break;
                }
            }

            if (!node) {
                x_ERROR("MinimumEnergyConsumption: Can not schedule the operator. No free resource available capacity is={}",
                          sinkNode->getRemainingCpuCapacity());
                throw std::runtime_error("Can not schedule the operator. No free resource available.");
            }

            if (executionPlanPtr->hasVertex(node->getId())) {

                x_DEBUG("MinimumEnergyConsumption: node {} was already used by other deployment", node->toString());

                const ExecutionNodePtr existingExecutionNode = executionPlanPtr->getExecutionNode(node->getId());

                stringstream operatorName;
                operatorName << existingExecutionNode->getOperatorName() << "=>" << targetOperator->getId() << "(OP-"
                             << std::to_string(targetOperator->getId()) << ")";
                existingExecutionNode->setOperatorName(operatorName.str());
                existingExecutionNode->addOperatorId(targetOperator->getId());
                existingExecutionNode->addOperator(legacyOperator->copy());
            } else {

                x_DEBUG("MinimumEnergyConsumption: create new execution node {}", node->toString());

                stringstream operatorName;
                operatorName << targetOperator->toString() << "(OP-" << std::to_string(targetOperator->getId()) << ")";

                // Create a new execution node
                const ExecutionNodePtr newExecutionNode = executionPlanPtr->createExecutionNode(operatorName.str(),
                                                                                                to_string(node->getId()),
                                                                                                node,
                                                                                                legacyOperator->copy());
                newExecutionNode->addOperatorId(targetOperator->getId());
            }

            node->reduceCpuCapacity(1);
            targetOperator = targetOperator->getParents()[0]->as<LogicalOperatorNode>();
            x_DEBUG("MinimumEnergyConsumption: Transforming New Operator into legacy operator");
            legacyOperator = translator->transform(targetOperator);
        }
    }

    x_DEBUG("MinimumEnergyConsumption: find the operator chain after the last placed operator");
    LogicalOperatorNodePtr nextSrcOptr = sourceOperator;
    while (nextSrcOptr->getId() != lastPlacedOperatorId) {
        nextSrcOptr = nextSrcOptr->getParents()[0]->as<LogicalOperatorNode>();
    }

    x_DEBUG("MinimumEnergyConsumption: Place remaining operator chain on common path");
    nextSrcOptr = nextSrcOptr->getParents()[0]->as<LogicalOperatorNode>();
    while (nextSrcOptr) {
        xTopologyEntryPtr node = nullptr;

        if (nextSrcOptr->instanceOf<SinkLogicalOperatorNode>()) {
            node = commonPath.back();
        } else {
            for (xTopologyEntryPtr pathNode : commonPath) {
                if (pathNode->getRemainingCpuCapacity() > 0) {
                    node = pathNode;
                    break;
                }
            }
        }

        if (!node) {
            x_THROW_RUNTIME_ERROR(
                "MinimumEnergyConsumption: Can not schedule the operator. No free resource available capacity is="
                + sinkNode->getRemainingCpuCapacity());
        }

        x_DEBUG("MinimumEnergyConsumption: Transforming New Operator into legacy operator");
        OperatorPtr legacyOperator = translator->transform(nextSrcOptr);

        if (executionPlanPtr->hasVertex(node->getId())) {

            x_DEBUG("MinimumEnergyConsumption: node {} was already used by other deployment", node->toString());

            const ExecutionNodePtr existingExecutionNode = executionPlanPtr->getExecutionNode(node->getId());

            stringstream operatorName;
            operatorName << existingExecutionNode->getOperatorName() << "=>" << nextSrcOptr->toString() << "(OP-"
                         << std::to_string(nextSrcOptr->getId()) << ")";
            existingExecutionNode->addOperator(legacyOperator->copy());
            existingExecutionNode->setOperatorName(operatorName.str());
            existingExecutionNode->addOperatorId(nextSrcOptr->getId());
        } else {

            x_DEBUG("MinimumEnergyConsumption: create new execution node {}", node->toString());

            stringstream operatorName;
            operatorName << nextSrcOptr->toString() << "(OP-" << std::to_string(nextSrcOptr->getId()) << ")";

            // Create a new execution node
            const ExecutionNodePtr newExecutionNode =
                executionPlanPtr->createExecutionNode(operatorName.str(), to_string(node->getId()), node, legacyOperator->copy());
            newExecutionNode->addOperatorId(nextSrcOptr->getId());
        }
        node->reduceCpuCapacity(1);
        nextSrcOptr = nextSrcOptr->getParents()[0]->as<LogicalOperatorNode>();
    }
}

}// namespace x
