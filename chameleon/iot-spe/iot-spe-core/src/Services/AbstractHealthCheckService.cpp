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

#include <Nodes/Util/Iterators/DepthFirstNodeIterator.hpp>
#include <Services/AbstractHealthCheckService.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>

namespace x {
AbstractHealthCheckService::AbstractHealthCheckService() {}

void AbstractHealthCheckService::stopHealthCheck() {
    x_DEBUG("AbstractHealthCheckService::stopHealthCheck called on id= {}", id);
    auto expected = true;
    if (!isRunning.compare_exchange_strong(expected, false)) {
        x_DEBUG("AbstractHealthCheckService::stopHealthCheck health check already stopped");
        return;
    }
    {
        std::unique_lock<std::mutex> lk(cvMutex);
        cv.notify_all();
    }
    auto ret = shutdownRPC->get_future().get();
    x_ASSERT(ret, "fail to shutdown health check");

    if (healthCheckingThread->joinable()) {
        healthCheckingThread->join();
        healthCheckingThread.reset();
        x_DEBUG("AbstractHealthCheckService::stopHealthCheck successfully stopped");
    } else {
        x_ERROR("HealthCheckService: health thread not joinable");
        x_THROW_RUNTIME_ERROR("Error while stopping healthCheckingThread->join");
    }
}

void AbstractHealthCheckService::addNodeToHealthCheck(TopologyNodePtr node) {
    x_DEBUG("HealthCheckService: adding node with id {}", node->getId());
    auto exists = nodeIdToTopologyNodeMap.contains(node->getId());
    if (exists) {
        x_THROW_RUNTIME_ERROR("HealthCheckService want to add node that already exists id=" << node->getId());
    }
    nodeIdToTopologyNodeMap.insert(node->getId(), node);
}

void AbstractHealthCheckService::removeNodeFromHealthCheck(TopologyNodePtr node) {
    auto exists = nodeIdToTopologyNodeMap.contains(node->getId());
    if (!exists) {
        x_THROW_RUNTIME_ERROR("HealthCheckService want to remove a node that does not exists id=" << node->getId());
    }
    x_DEBUG("HealthCheckService: removing node with id {}", node->getId());
    nodeIdToTopologyNodeMap.erase(node->getId());
}

bool AbstractHealthCheckService::getRunning() { return isRunning; }

bool AbstractHealthCheckService::isWorkerInactive(TopologyNodeId workerId) {
    x_DEBUG("HealthCheckService: checking if node with id {} is inactive", workerId);
    std::lock_guard<std::mutex> lock(cvMutex);
    bool isNotActive = inactiveWorkers.contains(workerId);
    if (isNotActive) {
        x_DEBUG("HealthCheckService: node with id {} is inactive", workerId);
        return true;
    }
    x_DEBUG("HealthCheckService: node with id {} is active", workerId);
    return false;
}

TopologyNodePtr AbstractHealthCheckService::getWorkerByWorkerId(TopologyNodeId workerId) {
    for (auto node : nodeIdToTopologyNodeMap.lock_table()) {
        if (node.first == workerId) {
            x_DEBUG("AbstractHealthCheckService: Found worker with id {}", workerId);
            return node.second;
        }
    }
    return nullptr;
}

}// namespace x