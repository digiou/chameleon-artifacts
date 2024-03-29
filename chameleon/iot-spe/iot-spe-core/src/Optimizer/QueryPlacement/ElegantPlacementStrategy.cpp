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

#include <API/Schema.hpp>
#include <Exceptions/QueryPlacementException.hpp>
#include <Operators/LogicalOperators/OpenCLLogicalOperatorNode.hpp>
#include <Operators/OperatorNode.hpp>
#include <Optimizer/QueryPlacement/ElegantPlacementStrategy.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <cpr/api.h>
#include <queue>
#include <utility>

namespace x::Optimizer {

std::unique_ptr<ElegantPlacementStrategy>
ElegantPlacementStrategy::create(const std::string& serviceURL,
                                 PlacementStrategy placementStrategy,
                                 x::GlobalExecutionPlanPtr globalExecutionPlan,
                                 x::TopologyPtr topology,
                                 x::Optimizer::TypeInferencePhasePtr typeInferencePhase) {

    float timeWeight = 0.0;

    switch (placementStrategy) {
        case PlacementStrategy::ELEGANT_PERFORMANCE: timeWeight = 1; break;
        case PlacementStrategy::ELEGANT_ENERGY: timeWeight = 0; break;
        case PlacementStrategy::ELEGANT_BALANCED: timeWeight = 0.5; break;
        default: x_ERROR("Unknown placement strategy for elegant {}", magic_enum::enum_name(placementStrategy));
    }

    return std::make_unique<ElegantPlacementStrategy>(ElegantPlacementStrategy(serviceURL,
                                                                               timeWeight,
                                                                               std::move(globalExecutionPlan),
                                                                               std::move(topology),
                                                                               std::move(typeInferencePhase)));
}

ElegantPlacementStrategy::ElegantPlacementStrategy(std::string serviceURL,
                                                   float timeWeight,
                                                   x::GlobalExecutionPlanPtr globalExecutionPlan,
                                                   x::TopologyPtr topology,
                                                   x::Optimizer::TypeInferencePhasePtr typeInferencePhase)
    : BasePlacementStrategy(std::move(globalExecutionPlan), std::move(topology), std::move(typeInferencePhase)),
      serviceURL(std::move(serviceURL)), timeWeight(timeWeight) {}

bool ElegantPlacementStrategy::updateGlobalExecutionPlan(QueryId queryId,
                                                         FaultToleranceType faultToleranceType,
                                                         LineageType lineageType,
                                                         const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                                         const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) {

    try {

        //1. Make rest call to get the external operator placement service.
        //prepare request payload
        nlohmann::json payload{};

        //1.a: Get query plan as json
        auto queryGraph = prepareQueryPayload(pinnedDownStreamOperators, pinnedUpStreamOperators);
        payload.push_back(queryGraph);

        // 1.b: Get topology and network information as json
        auto availableNodes = prepareTopologyPayload();
        payload.push_back(availableNodes);

        // 1.c: Optimization objective
        payload[TIME_WEIGHT_KEY] = timeWeight;

        //1.d: Make a rest call to elegant planner
        cpr::Response response = cpr::Post(cpr::Url{serviceURL},
                                           cpr::Header{{"Content-Type", "application/json"}},
                                           cpr::Body{payload.dump()},
                                           cpr::Timeout(ELEGANT_SERVICE_TIMEOUT));
        if (response.status_code != 200) {
            throw Exceptions::QueryPlacementException(
                queryId,
                "ElegantPlacementStrategy::updateGlobalExecutionPlan: Error in call to Elegant planner with code "
                    + std::to_string(response.status_code) + " and msg " + response.reason);
        }

        //2. Parse the response of the external placement service
        pinOperatorsBasedOnElegantService(queryId, pinnedDownStreamOperators, response);

        // 3. Place the operators
        placePinnedOperators(queryId, pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 4. add network source and sink operators
        addNetworkSourceAndSinkOperators(queryId, pinnedUpStreamOperators, pinnedDownStreamOperators);

        // 5. Perform type inference on updated query plans
        return runTypeInferencePhase(queryId, faultToleranceType, lineageType);
    } catch (const std::exception& ex) {
        throw Exceptions::QueryPlacementException(queryId, ex.what());
    }
}

void ElegantPlacementStrategy::pinOperatorsBasedOnElegantService(
    QueryId queryId,
    const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators,
    cpr::Response& response) const {
    nlohmann::json jsonResponse = nlohmann::json::parse(response.text);
    //Fetch the placement data
    auto placementData = jsonResponse[PLACEMENT_KEY];

    // fill with true where nodeId is present
    for (const auto& placement : placementData) {
        OperatorId operatorId = placement[OPERATOR_ID_KEY];
        TopologyNodeId topologyNodeId = placement[NODE_ID_KEY];

        bool pinned = false;
        for (const auto& item : pinnedDownStreamOperators) {
            auto operatorToPin = item->getChildWithOperatorId(operatorId)->as<OperatorNode>();
            if (operatorToPin) {
                operatorToPin->addProperty(PINNED_NODE_ID, topologyNodeId);

                if (operatorToPin->instanceOf<OpenCLLogicalOperatorNode>()) {
                    std::string deviceId = placement[DEVICE_ID_KEY];
                    operatorToPin->as<OpenCLLogicalOperatorNode>()->setDeviceId(deviceId);
                }

                pinned = true;
                break;
            }
        }

        if (!pinned) {
            throw Exceptions::QueryPlacementException(queryId,
                                                      "ElegantPlacementStrategy: Unable to find operator with id "
                                                          + std::to_string(operatorId) + " in the given list of operators.");
        }
    }
}

nlohmann::json ElegantPlacementStrategy::prepareQueryPayload(const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                                             const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) {

    x_DEBUG("ElegantPlacementStrategy: Getting the json representation of the query plan");

    nlohmann::json queryPlan{};
    std::vector<nlohmann::json> nodes{};

    std::set<OperatorNodePtr> visitedOperator;
    std::queue<OperatorNodePtr> operatorsToVisit;
    //initialize with upstream operators
    for (const auto& pinnedDownStreamOperator : pinnedDownStreamOperators) {
        operatorsToVisit.emplace(pinnedDownStreamOperator);
    }

    while (!operatorsToVisit.empty()) {

        auto logicalOperator = operatorsToVisit.front();//fetch the front operator
        operatorsToVisit.pop();                         //pop the front operator

        //if operator was not previously visited
        if (visitedOperator.insert(logicalOperator).second) {
            nlohmann::json node;
            node[OPERATOR_ID_KEY] = logicalOperator->getId();
            auto pinnedNodeId = logicalOperator->getProperty(PINNED_NODE_ID);
            node[CONSTRAINT_KEY] = pinnedNodeId.has_value() ? std::any_cast<std::string>(pinnedNodeId) : EMPTY_STRING;
            auto sourceCode = logicalOperator->getProperty(SOURCE_CODE);
            node[SOURCE_CODE] = sourceCode.has_value() ? std::any_cast<std::string>(sourceCode) : EMPTY_STRING;
            nodes.push_back(node);
            node[INPUT_DATA_KEY] = logicalOperator->getOutputSchema()->getSchemaSizeInBytes();

            auto found = std::find_if(pinnedUpStreamOperators.begin(),
                                      pinnedUpStreamOperators.end(),
                                      [logicalOperator](const OperatorNodePtr& pinnedOperator) {
                                          return pinnedOperator->getId() == logicalOperator->getId();
                                      });

            //array of upstream operator ids
            auto upstreamOperatorIds = nlohmann::json::array();
            //Only explore further upstream operators if this operator is not in the list of pinned upstream operators
            if (found == pinnedUpStreamOperators.end()) {
                for (const auto& upstreamOperator : logicalOperator->getChildren()) {
                    upstreamOperatorIds.push_back(upstreamOperator->as<OperatorNode>()->getId());
                    operatorsToVisit.emplace(upstreamOperator->as<OperatorNode>());// add children for future visit
                }
            }
            node[CHILDREN_KEY] = upstreamOperatorIds;
        }
    }
    queryPlan[OPERATOR_GRAPH_KEY] = nodes;
    return queryPlan;
}

nlohmann::json ElegantPlacementStrategy::prepareTopologyPayload() {
    x_DEBUG("ElegantPlacementStrategy: Getting the json representation of available nodes");
    nlohmann::json topologyJson{};
    auto root = topology->getRoot();
    std::deque<TopologyNodePtr> parentToAdd{std::move(root)};
    std::deque<TopologyNodePtr> childToAdd;

    std::vector<nlohmann::json> nodes = {};
    std::vector<nlohmann::json> edges = {};

    while (!parentToAdd.empty()) {
        // Current topology node to add to the JSON
        TopologyNodePtr currentNode = parentToAdd.front();
        nlohmann::json currentNodeJsonValue{};
        parentToAdd.pop_front();

        // Add properties for current topology node
        currentNodeJsonValue[NODE_ID_KEY] = currentNode->getId();
        currentNodeJsonValue[NODE_TYPE_KEY] = "stationary";// always set to stationary

        std::vector<nlohmann::json> devices = {};
        //TODO: a node can have multiple devices. At this point it is not clear how these information will come to us.
        // should be handled as part of issue #3853
        nlohmann::json currentNodeOpenCLJsonValue{};
        currentNodeOpenCLJsonValue[DEVICE_ID_KEY] = std::any_cast<std::string>(currentNode->getNodeProperty("DEVICE_ID"));
        currentNodeOpenCLJsonValue[DEVICE_TYPE_KEY] = std::any_cast<std::string>(currentNode->getNodeProperty("DEVICE_TYPE"));
        currentNodeOpenCLJsonValue[DEVICE_NAME_KEY] = std::any_cast<std::string>(currentNode->getNodeProperty("DEVICE_NAME"));
        currentNodeOpenCLJsonValue[MEMORY_KEY] = std::any_cast<uint64_t>(currentNode->getNodeProperty("DEVICE_MEMORY"));
        devices.push_back(currentNodeOpenCLJsonValue);

        currentNodeJsonValue[DEVICES_KEY] = devices;

        auto children = currentNode->getChildren();
        for (const auto& child : children) {
            // Add edge information for current topology node
            nlohmann::json currentEdgeJsonValue{};
            currentEdgeJsonValue[LINK_ID_KEY] =
                std::to_string(child->as<TopologyNode>()->getId()) + "-" + std::to_string(currentNode->getId());
            currentEdgeJsonValue[TRANSFER_RATE_KEY] = 100;// FIXME: replace it with more intelligible value
            edges.push_back(currentEdgeJsonValue);
            childToAdd.push_back(child->as<TopologyNode>());
        }

        if (parentToAdd.empty()) {
            parentToAdd.insert(parentToAdd.end(), childToAdd.begin(), childToAdd.end());
            childToAdd.clear();
        }

        nodes.push_back(currentNodeJsonValue);
    }
    x_INFO("ElegantPlacementStrategy: no more topology nodes to add");

    topologyJson[AVAILABLE_NODES_KEY] = nodes;
    topologyJson[NETWORK_DELAYS_KEY] = edges;
    return topologyJson;
}

}// namespace x::Optimizer