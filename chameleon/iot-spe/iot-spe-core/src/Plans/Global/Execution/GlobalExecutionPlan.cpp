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

#include <Nodes/Util/ConsoleDumpHandler.hpp>
#include <Nodes/Util/DumpContext.hpp>
#include <Plans/Global/Execution/ExecutionNode.hpp>
#include <Plans/Global/Execution/GlobalExecutionPlan.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>
#include <algorithm>

namespace x {

GlobalExecutionPlanPtr GlobalExecutionPlan::create() { return std::make_shared<GlobalExecutionPlan>(); }

bool GlobalExecutionPlan::checkIfExecutionNodeExists(uint64_t id) {
    x_DEBUG("GlobalExecutionPlan: Checking if Execution node with id {} exists", id);
    return nodeIdIndex.find(id) != nodeIdIndex.end();
}

bool GlobalExecutionPlan::checkIfExecutionNodeIsARoot(uint64_t id) {
    x_DEBUG("GlobalExecutionPlan: Checking if Execution node with id {} is a root node", id);
    return std::find(rootNodes.begin(), rootNodes.end(), getExecutionNodeByNodeId(id)) != rootNodes.end();
}

ExecutionNodePtr GlobalExecutionPlan::getExecutionNodeByNodeId(uint64_t id) {
    if (checkIfExecutionNodeExists(id)) {
        x_DEBUG("GlobalExecutionPlan: Returning execution node with id  {}", id);
        return nodeIdIndex[id];
    }
    x_WARNING("GlobalExecutionPlan: Execution node doesn't exists with the id {}", id);
    return nullptr;
}

bool GlobalExecutionPlan::addExecutionNodeAsParentTo(uint64_t childId, const ExecutionNodePtr& parentExecutionNode) {
    ExecutionNodePtr childNode = getExecutionNodeByNodeId(childId);
    if (childNode) {
        x_DEBUG("GlobalExecutionPlan: Adding Execution node as parent to the execution node with id  {}", childId);
        if (childNode->containAsParent(parentExecutionNode)) {
            x_DEBUG("GlobalExecutionPlan: Execution node is already a parent to the node with id  {}", childId);
            return true;
        }

        if (childNode->addParent(parentExecutionNode)) {
            x_DEBUG("GlobalExecutionPlan: Added Execution node with id  {}", parentExecutionNode->getId());
            nodeIdIndex[parentExecutionNode->getId()] = parentExecutionNode;
            return true;
        }
        x_WARNING("GlobalExecutionPlan: Failed to add Execution node as parent to the execution node with id {}", childId);
        return false;
    }
    x_WARNING("GlobalExecutionPlan: Child node doesn't exists with the id {}", childId);
    return false;
}

bool GlobalExecutionPlan::addExecutionNodeAsRoot(const ExecutionNodePtr& executionNode) {
    x_DEBUG("GlobalExecutionPlan: Added Execution node as root node");
    auto found = std::find(rootNodes.begin(), rootNodes.end(), executionNode);
    if (found == rootNodes.end()) {
        rootNodes.push_back(executionNode);
        x_DEBUG("GlobalExecutionPlan: Added Execution node with id  {}", executionNode->getId());
        nodeIdIndex[executionNode->getId()] = executionNode;
    } else {
        x_WARNING("GlobalExecutionPlan: Execution node already present in the root node list");
    }
    return true;
}

bool GlobalExecutionPlan::addExecutionNode(const ExecutionNodePtr& executionNode) {
    x_DEBUG("GlobalExecutionPlan: Added Execution node with id  {}", executionNode->getId());
    nodeIdIndex[executionNode->getId()] = executionNode;
    scheduleExecutionNode(executionNode);
    return true;
}

bool GlobalExecutionPlan::removeExecutionNode(uint64_t id) {
    x_DEBUG("GlobalExecutionPlan: Removing Execution node with id  {}", id);
    if (checkIfExecutionNodeExists(id)) {
        x_DEBUG("GlobalExecutionPlan: Removed execution node with id  {}", id);
        auto found = std::find_if(rootNodes.begin(), rootNodes.end(), [id](const ExecutionNodePtr& rootNode) {
            return rootNode->getId() == id;
        });
        if (found != rootNodes.end()) {
            rootNodes.erase(found);
        }
        return nodeIdIndex.erase(id) == 1;
    }
    x_DEBUG("GlobalExecutionPlan: Failed to remove Execution node with id  {}", id);
    return false;
}

bool GlobalExecutionPlan::removeQuerySubPlans(QueryId queryId) {
    auto itr = queryIdIndex.find(queryId);
    if (itr == queryIdIndex.end()) {
        x_DEBUG("GlobalExecutionPlan: No query with id {} exists in the system", queryId);
        return false;
    }

    std::vector<ExecutionNodePtr> executionNodes = queryIdIndex[queryId];
    x_DEBUG("GlobalExecutionPlan: Found {} Execution node for query with id {}", executionNodes.size(), queryId);
    for (const auto& executionNode : executionNodes) {
        uint64_t executionNodeId = executionNode->getId();
        if (!executionNode->removeQuerySubPlans(queryId)) {
            x_ERROR("GlobalExecutionPlan: Unable to remove query sub plan with id {} from execution node with id {}",
                      queryId,
                      executionNodeId);
            return false;
        }
        if (executionNode->getAllQuerySubPlans().empty()) {
            removeExecutionNode(executionNodeId);
        }
    }
    queryIdIndex.erase(queryId);
    x_DEBUG("GlobalExecutionPlan: Removed all Execution nodes for Query with id  {}", queryId);
    return true;
}

std::vector<ExecutionNodePtr> GlobalExecutionPlan::getExecutionNodesByQueryId(QueryId queryId) {
    auto itr = queryIdIndex.find(queryId);
    if (itr != queryIdIndex.end()) {
        x_DEBUG("GlobalExecutionPlan: Returning vector of Execution nodes for the query with id  {}", queryId);
        return itr->second;
    }
    x_WARNING("GlobalExecutionPlan: unable to find the Execution nodes for the query with id {}", queryId);
    return {};
}

std::vector<ExecutionNodePtr> GlobalExecutionPlan::getAllExecutionNodes() {
    x_INFO("GlobalExecutionPlan: get all execution nodes");
    std::vector<ExecutionNodePtr> executionNodes;
    for (auto& [nodeId, executionNode] : nodeIdIndex) {
        executionNodes.push_back(executionNode);
    }
    return executionNodes;
}

std::vector<ExecutionNodePtr> GlobalExecutionPlan::getExecutionNodesToSchedule() {
    x_DEBUG("GlobalExecutionPlan: Returning vector of Execution nodes to be scheduled");
    return executionNodesToSchedule;
}

std::vector<ExecutionNodePtr> GlobalExecutionPlan::getRootNodes() {
    x_DEBUG("GlobalExecutionPlan: Get root nodes of the execution plan");
    return rootNodes;
}

std::string GlobalExecutionPlan::getAsString() {
    x_DEBUG("GlobalExecutionPlan: Get Execution plan as string");
    std::stringstream ss;
    auto dumpHandler = ConsoleDumpHandler::create(ss);
    for (const auto& rootNode : rootNodes) {
        dumpHandler->multilineDump(rootNode);
    }
    return ss.str();
}

void GlobalExecutionPlan::scheduleExecutionNode(const ExecutionNodePtr& executionNode) {
    x_DEBUG("GlobalExecutionPlan: Schedule execution node for deployment");
    auto found = std::find(executionNodesToSchedule.begin(), executionNodesToSchedule.end(), executionNode);
    if (found != executionNodesToSchedule.end()) {
        x_DEBUG("GlobalExecutionPlan: Execution node {} marked as to be scheduled", executionNode->getId());
        executionNodesToSchedule.push_back(executionNode);
    } else {
        x_WARNING("GlobalExecutionPlan: Execution node {} already scheduled", executionNode->getId());
    }
    mapExecutionNodeToQueryId(executionNode);
}

void GlobalExecutionPlan::mapExecutionNodeToQueryId(const ExecutionNodePtr& executionNode) {
    x_DEBUG("GlobalExecutionPlan: Mapping execution node {} to the query Id index.", executionNode->getId());
    auto querySubPlans = executionNode->getAllQuerySubPlans();
    for (const auto& pair : querySubPlans) {
        QueryId queryId = pair.first;
        if (queryIdIndex.find(queryId) == queryIdIndex.end()) {
            x_DEBUG("GlobalExecutionPlan: Query Id {} does not exists adding a new entry with execution node {}",
                      queryId,
                      executionNode->getId());
            queryIdIndex[queryId] = {executionNode};
        } else {
            std::vector<ExecutionNodePtr> executionNodes = queryIdIndex[queryId];
            auto found = std::find(executionNodes.begin(), executionNodes.end(), executionNode);
            if (found == executionNodes.end()) {
                x_DEBUG("GlobalExecutionPlan: Adding execution node {} to the query Id {}", executionNode->getId(), queryId);
                executionNodes.push_back(executionNode);
                queryIdIndex[queryId] = executionNodes;
            } else {
                x_DEBUG("GlobalExecutionPlan: Skipping as execution node {} already mapped to the query Id {}",
                          executionNode->getId(),
                          queryId);
            }
        }
    }
}

std::map<uint64_t, uint32_t> GlobalExecutionPlan::getMapOfTopologyNodeIdToOccupiedResource(QueryId queryId) {

    x_INFO("GlobalExecutionPlan: Get a map of occupied resources for the query {}", queryId);
    std::map<uint64_t, uint32_t> mapOfTopologyNodeIdToOccupiedResources;
    std::vector<ExecutionNodePtr> executionNodes = queryIdIndex[queryId];
    x_DEBUG("GlobalExecutionPlan: Found {} Execution node for query with id {}", executionNodes.size(), queryId);
    for (auto& executionNode : executionNodes) {
        uint32_t occupiedResource = executionNode->getOccupiedResources(queryId);
        mapOfTopologyNodeIdToOccupiedResources[executionNode->getTopologyNode()->getId()] = occupiedResource;
    }
    x_DEBUG("GlobalExecutionPlan: returning the map of occupied resources for the query  {}", queryId);
    return mapOfTopologyNodeIdToOccupiedResources;
}

}// namespace x
