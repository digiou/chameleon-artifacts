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
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/LogicalSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Operators/Operator.hpp>
#include <Optimizer/ExecutionGraph.hpp>
#include <Optimizer/xExecutionPlan.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QueryPlacement/LowLatencyStrategy.hpp>
#include <Optimizer/Utils/PathFinder.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/CodeGenerator/TranslateToLegacyExpression.hpp>
#include <Topology/xTopologyGraph.hpp>
#include <Topology/xTopologyPlan.hpp>
#include <Util/Logger/Logger.hpp>

namespace x {

LowLatencyStrategy::LowLatencyStrategy(xTopologyPlanPtr xTopologyPlan) : BasePlacementStrategy(xTopologyPlan) {}

xExecutionPlanPtr LowLatencyStrategy::initializeExecutionPlan(QueryPlanPtr queryPlan,
                                                                xTopologyPlanPtr xTopologyPlan,
                                                                Catalogs::Source::SourceCatalogPtr sourceCatalog) {
    this->xTopologyPlan = xTopologyPlan;
    const SourceLogicalOperatorNodePtr sourceOperator = queryPlan->getSourceOperators()[0];

    // FIXME: current implementation assumes that we have only one source and therefore only one source operator.
    const string sourceName = queryPlan->getSourceName();

    if (!sourceOperator) {
        x_THROW_RUNTIME_ERROR("LowLatency: Unable to find the source operator.");
    }

    const std::vector<xTopologyEntryPtr> sourceNodes = sourceCatalog->getSourceNodesForLogicalSource(sourceName);

    if (sourceNodes.empty()) {
        x_ERROR("LowLatency: Unable to find the target source:  {}", sourceName);
        throw std::runtime_error("No source found in the topology for source " + sourceName);
    }

    xExecutionPlanPtr xExecutionPlanPtr = std::make_shared<xExecutionPlan>();
    const xTopologyGraphPtr xTopologyGraphPtr = xTopologyPlan->getxTopologyGraph();

    x_INFO("LowLatency: Placing operators on the x topology.");
    placeOperators(xExecutionPlanPtr, xTopologyGraphPtr, sourceOperator, sourceNodes);

    xTopologyEntryPtr rootNode = xTopologyGraphPtr->getRoot();

    x_DEBUG("LowLatency: Find the path used for performing the placement based on the strategy type");
    vector<xTopologyEntryPtr> candidateNodes = getCandidateNodesForFwdOperatorPlacement(sourceNodes, rootNode);

    x_INFO("LowLatency: Adding forward operators.");
    addSystemGeneratedOperators(candidateNodes, xExecutionPlanPtr);

    x_INFO("LowLatency: Generating complete execution Graph.");
    fillExecutionGraphWithTopologyInformation(xExecutionPlanPtr);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    SchemaPtr schema = sourceOperator->getSourceDescriptor()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, xExecutionPlanPtr);

    return xExecutionPlanPtr;
}

vector<xTopologyEntryPtr>
LowLatencyStrategy::getCandidateNodesForFwdOperatorPlacement(const vector<xTopologyEntryPtr>& sourceNodes,
                                                             const xTopologyEntryPtr rootNode) const {

    vector<xTopologyEntryPtr> candidateNodes;
    for (xTopologyEntryPtr targetSource : sourceNodes) {
        //Find the list of nodes connecting the source and destination nodes
        std::vector<xTopologyEntryPtr> nodesOnPath = pathFinder->findPathWithMinLinkLatency(targetSource, rootNode);
        candidateNodes.insert(candidateNodes.end(), nodesOnPath.begin(), nodesOnPath.end());
    }

    return candidateNodes;
}

void LowLatencyStrategy::placeOperators(xExecutionPlanPtr executionPlanPtr,
                                        xTopologyGraphPtr xTopologyGraphPtr,
                                        LogicalOperatorNodePtr sourceOperator,
                                        vector<xTopologyEntryPtr> sourceNodes) {

    TranslateToLegacyPlanPhasePtr translator = TranslateToLegacyPlanPhase::create();

    const xTopologyEntryPtr sinkNode = xTopologyGraphPtr->getRoot();
    for (xTopologyEntryPtr sourceNode : sourceNodes) {

        LogicalOperatorNodePtr targetOperator = sourceOperator;
        const vector<xTopologyEntryPtr> targetPath = pathFinder->findPathWithMinLinkLatency(sourceNode, sinkNode);

        for (xTopologyEntryPtr node : targetPath) {
            while (node->getRemainingCpuCapacity() > 0 && targetOperator) {

                x_DEBUG("TopDown: Transforming New Operator into legacy operator");
                OperatorPtr legacyOperator = translator->transform(targetOperator);

                if (targetOperator->instanceOf<SinkLogicalOperatorNode>()) {
                    node = sinkNode;
                }

                if (!executionPlanPtr->hasVertex(node->getId())) {
                    x_DEBUG("LowLatency: Create new execution node.");
                    stringstream operatorName;
                    operatorName << targetOperator->toString() << "(OP-" << std::to_string(targetOperator->getId()) << ")";
                    const ExecutionNodePtr newExecutionNode = executionPlanPtr->createExecutionNode(operatorName.str(),
                                                                                                    to_string(node->getId()),
                                                                                                    node,
                                                                                                    legacyOperator->copy());
                    newExecutionNode->addOperatorId(legacyOperator->getOperatorId());
                } else {

                    const ExecutionNodePtr existingExecutionNode = executionPlanPtr->getExecutionNode(node->getId());
                    uint64_t operatorId = legacyOperator->getOperatorId();
                    vector<uint64_t>& residentOperatorIds = existingExecutionNode->getChildOperatorIds();
                    const auto exists = std::find(residentOperatorIds.begin(), residentOperatorIds.end(), operatorId);
                    if (exists != residentOperatorIds.end()) {
                        //skip adding rest of the operator chains as they already exists.
                        x_DEBUG("LowLatency: skip adding rest of the operator chains as they already exists.");
                        targetOperator = nullptr;
                        break;
                    } else {

                        x_DEBUG("LowLatency: adding target operator to already existing operator chain.");
                        stringstream operatorName;
                        operatorName << existingExecutionNode->getOperatorName() << "=>" << targetOperator->toString() << "(OP-"
                                     << std::to_string(targetOperator->getId()) << ")";
                        existingExecutionNode->addOperator(legacyOperator->copy());
                        existingExecutionNode->setOperatorName(operatorName.str());
                        existingExecutionNode->addOperatorId(legacyOperator->getOperatorId());
                    }
                }

                targetOperator = targetOperator->getParents()[0]->as<LogicalOperatorNode>();
                node->reduceCpuCapacity(1);
            }

            if (!targetOperator) {
                break;
            }
        }
    }
}

}// namespace x
