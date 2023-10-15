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
#include <Optimizer/QueryPlacement/MinimumResourceConsumptionStrategy.hpp>
#include <Optimizer/Utils/PathFinder.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/CodeGenerator/TranslateToLegacyExpression.hpp>
#include <Topology/xTopologyPlan.hpp>
#include <Util/Logger/Logger.hpp>

namespace x {

MinimumResourceConsumptionStrategy::MinimumResourceConsumptionStrategy(xTopologyPlanPtr xTopologyPlan)
    : BasePlacementStrategy(xTopologyPlan) {}

xExecutionPlanPtr
MinimumResourceConsumptionStrategy::initializeExecutionPlan(QueryPlanPtr queryPlan,
                                                            xTopologyPlanPtr xTopologyPlan,
                                                            Catalogs::Source::SourceCatalogPtr sourceCatalog) {
    this->xTopologyPlan = xTopologyPlan;
    const SourceLogicalOperatorNodePtr sourceOperator = queryPlan->getSourceOperators()[0];

    // FIXME: current implementation assumes that we have only one source and therefore only one source operator.
    const string sourceName = queryPlan->getSourceName();

    if (!sourceOperator) {
        x_THROW_RUNTIME_ERROR("MinimumResourceConsumption: Unable to find the source operator.");
    }

    const vector<xTopologyEntryPtr> sourceNodes = sourceCatalog->getSourceNodesForLogicalSource(sourceName);

    if (sourceNodes.empty()) {
        x_THROW_RUNTIME_ERROR("MinimumResourceConsumption: Unable to find the target source: " + sourceName);
    }

    xExecutionPlanPtr xExecutionPlanPtr = std::make_shared<xExecutionPlan>();
    const xTopologyGraphPtr xTopologyGraphPtr = xTopologyPlan->getxTopologyGraph();

    x_INFO("MinimumResourceConsumption: Placing operators on the x topology.");
    placeOperators(xExecutionPlanPtr, xTopologyGraphPtr, sourceOperator, sourceNodes);

    xTopologyEntryPtr rootNode = xTopologyGraphPtr->getRoot();

    x_DEBUG("MinimumResourceConsumption: Find the path used for performing the placement based on the strategy type");
    vector<xTopologyEntryPtr> candidateNodes = getCandidateNodesForFwdOperatorPlacement(sourceNodes, rootNode);

    x_INFO("MinimumResourceConsumption: Adding forward operators.");
    addSystemGeneratedOperators(candidateNodes, xExecutionPlanPtr);

    x_INFO("MinimumResourceConsumption: Generating complete execution Graph.");
    fillExecutionGraphWithTopologyInformation(xExecutionPlanPtr);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    SchemaPtr schema = sourceOperator->getSourceDescriptor()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, xExecutionPlanPtr);

    return xExecutionPlanPtr;
}

vector<xTopologyEntryPtr>
MinimumResourceConsumptionStrategy::getCandidateNodesForFwdOperatorPlacement(const vector<xTopologyEntryPtr>& sourceNodes,
                                                                             const x::xTopologyEntryPtr rootNode) const {

    auto pathMap = pathFinder->findUniquePathBetween(sourceNodes, rootNode);
    vector<xTopologyEntryPtr> candidateNodes;
    for (auto [key, value] : pathMap) {
        candidateNodes.insert(candidateNodes.end(), value.begin(), value.end());
    }

    return candidateNodes;
}

void MinimumResourceConsumptionStrategy::placeOperators(xExecutionPlanPtr executionPlanPtr,
                                                        xTopologyGraphPtr xTopologyGraphPtr,
                                                        LogicalOperatorNodePtr sourceOperator,
                                                        vector<xTopologyEntryPtr> sourceNodes) {

    TranslateToLegacyPlanPhasePtr translator = TranslateToLegacyPlanPhase::create();
    const xTopologyEntryPtr sinkNode = xTopologyGraphPtr->getRoot();

    auto pathMap = pathFinder->findUniquePathBetween(sourceNodes, sinkNode);

    //Prepare list of ordered common nodes
    vector<vector<xTopologyEntryPtr>> listOfPaths;
    transform(pathMap.begin(), pathMap.end(), back_inserter(listOfPaths), [](const auto pair) {
        return pair.second;
    });

    vector<xTopologyEntryPtr> commonPath;

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

    x_DEBUG("MinimumResourceConsumption: Transforming New Operator into legacy operator");
    OperatorPtr legacySourceOperator = translator->transform(sourceOperator);

    for (xTopologyEntryPtr sourceNode : sourceNodes) {

        x_DEBUG("MinimumResourceConsumption: Create new execution node for source operator.");
        stringstream operatorName;
        operatorName << sourceOperator->toString() << "(OP-" << std::to_string(sourceOperator->getId()) << ")";

        const ExecutionNodePtr newExecutionNode = executionPlanPtr->createExecutionNode(operatorName.str(),
                                                                                        to_string(sourceNode->getId()),
                                                                                        sourceNode,
                                                                                        legacySourceOperator->copy());
        newExecutionNode->addOperatorId(sourceOperator->getId());
        sourceNode->reduceCpuCapacity(1);
    }

    LogicalOperatorNodePtr targetOperator = sourceOperator->getParents()[0]->as<LogicalOperatorNode>();

    while (targetOperator && targetOperator->instanceOf<SinkLogicalOperatorNode>()) {
        xTopologyEntryPtr node = nullptr;
        for (xTopologyEntryPtr commonNode : commonPath) {
            if (commonNode->getRemainingCpuCapacity() > 0) {
                node = commonNode;
                break;
            }
        }

        if (!node) {
            x_ERROR("MinimumResourceConsumption: Can not schedule the operator. No free resource available capacity is={}",
                      sinkNode->getRemainingCpuCapacity());
            throw std::runtime_error("Can not schedule the operator. No free resource available.");
        }

        x_DEBUG("MinimumResourceConsumption: suitable placement for operator {} is {}",
                  targetOperator->toString(),
                  node->toString());

        x_DEBUG("MinimumResourceConsumption: Transforming New Operator into legacy operator");
        OperatorPtr legacyOperator = translator->transform(targetOperator);

        // If the selected x node was already used by another operator for placement then do not create a
        // new execution node rather add operator to existing node.
        if (executionPlanPtr->hasVertex(node->getId())) {

            x_DEBUG("MinimumResourceConsumption: node {} was already used by other deployment", node->toString());

            const ExecutionNodePtr existingExecutionNode = executionPlanPtr->getExecutionNode(node->getId());

            stringstream operatorName;
            operatorName << existingExecutionNode->getOperatorName() << "=>" << targetOperator->toString() << "(OP-"
                         << std::to_string(targetOperator->getId()) << ")";
            existingExecutionNode->addOperator(legacyOperator->copy());
            existingExecutionNode->setOperatorName(operatorName.str());
            existingExecutionNode->addOperatorId(targetOperator->getId());
        } else {

            x_DEBUG("MinimumResourceConsumption: create new execution node {}", node->toString());

            stringstream operatorName;
            operatorName << targetOperator->toString() << "(OP-" << std::to_string(targetOperator->getId()) << ")";

            // Create a new execution node
            const ExecutionNodePtr newExecutionNode =
                executionPlanPtr->createExecutionNode(operatorName.str(), to_string(node->getId()), node, legacyOperator->copy());
            newExecutionNode->addOperatorId(targetOperator->getId());
        }

        // Reduce the processing capacity by 1
        // FIXME: Bring some logic here where the cpu capacity is reduced based on operator workload
        node->reduceCpuCapacity(1);

        if (!targetOperator->getParents().empty()) {
            targetOperator = targetOperator->getParents()[0]->as<LogicalOperatorNode>();
        }
    }

    if (sinkNode->getRemainingCpuCapacity() > 0) {
        x_DEBUG("MinimumResourceConsumption: Transforming New Operator into legacy operator");
        OperatorPtr legacyOperator = translator->transform(targetOperator);
        if (executionPlanPtr->hasVertex(sinkNode->getId())) {

            x_DEBUG("MinimumResourceConsumption: node {} was already used by other deployment", sinkNode->toString());

            const ExecutionNodePtr existingExecutionNode = executionPlanPtr->getExecutionNode(sinkNode->getId());

            stringstream operatorName;
            operatorName << existingExecutionNode->getOperatorName() << "=>" << targetOperator->toString() << "(OP-"
                         << std::to_string(targetOperator->getId()) << ")";
            existingExecutionNode->addOperator(legacyOperator->copy());
            existingExecutionNode->setOperatorName(operatorName.str());
            existingExecutionNode->addOperatorId(targetOperator->getId());
        } else {

            x_DEBUG("MinimumResourceConsumption: create new execution node {} with sink operator", sinkNode->toString());

            stringstream operatorName;
            operatorName << targetOperator->toString() << "(OP-" << std::to_string(targetOperator->getId()) << ")";

            // Create a new execution node
            const ExecutionNodePtr newExecutionNode = executionPlanPtr->createExecutionNode(operatorName.str(),
                                                                                            to_string(sinkNode->getId()),
                                                                                            sinkNode,
                                                                                            legacyOperator->copy());
            newExecutionNode->addOperatorId(targetOperator->getId());
        }
        sinkNode->reduceCpuCapacity(1);
    } else {
        x_ERROR("MinimumResourceConsumption: Can not schedule the operator. No free resource available capacity is={}",
                  sinkNode->getRemainingCpuCapacity());
        throw std::runtime_error("Can not schedule the operator. No free resource available.");
    }
}

}// namespace x
