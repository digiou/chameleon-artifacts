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

#ifndef x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_ELEGANTPLACEMENTSTRATEGY_HPP_
#define x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_ELEGANTPLACEMENTSTRATEGY_HPP_

#include <Optimizer/QueryPlacement/BasePlacementStrategy.hpp>
#include <Util/PlacementStrategy.hpp>
#include <cpr/response.h>
#include <nlohmann/json.hpp>

namespace x::Optimizer {

/**
 * @brief This class allows us to communicate with external planner service to perform operator placement.
 */
class ElegantPlacementStrategy : public BasePlacementStrategy {
  public:
    ~ElegantPlacementStrategy() override = default;

    /**
     * @brief Create instance of Elegant Placement Strategy
     * @param serviceURL: service URL to connect to
     * @param placementStrategy: The name of the ELEGANT placement strategy
     * @param globalExecutionPlan: the global execution plan
     * @param topology: the topology
     * @param typeInferencePhase: type inference phase
     * @return shared pointer to the placement strategy
     */
    static std::unique_ptr<ElegantPlacementStrategy> create(const std::string& serviceURL,
                                                            PlacementStrategy placementStrategy,
                                                            GlobalExecutionPlanPtr globalExecutionPlan,
                                                            TopologyPtr topology,
                                                            TypeInferencePhasePtr typeInferencePhase);

    bool updateGlobalExecutionPlan(QueryId queryId,
                                   FaultToleranceType faultToleranceType,
                                   LineageType lineageType,
                                   const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                   const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) override;

  private:
    explicit ElegantPlacementStrategy(std::string serviceURL,
                                      float timeWeight,
                                      GlobalExecutionPlanPtr globalExecutionPlan,
                                      TopologyPtr topology,
                                      TypeInferencePhasePtr typeInferencePhase);

    /**
     * @brief: Prepare the query payload for the external service
     * @param pinnedUpStreamOperators: pinned upstream operators of the plan to be placed
     * @param pinnedDownStreamOperators: pinned downstream operators of the plan to be placed
     * @return json representing the payload
     */
    nlohmann::json prepareQueryPayload(const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                       const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators);

    /**
     * @brief: Prepare the topology payload for the external placement service
     * @return json representing the payload
     */
    nlohmann::json prepareTopologyPayload();

    /**
     * @brief Add pinning information to the operators based on the response received from the external placement service
     * @param queryId: query id
     * @param pinnedDownStreamOperators: pinned downstream operators
     * @param response: Json representing the placement response received from the external service
     */
    void pinOperatorsBasedOnElegantService(QueryId queryId,
                                           const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators,
                                           cpr::Response& response) const;

    std::string serviceURL;
    float timeWeight;

    const int32_t ELEGANT_SERVICE_TIMEOUT = 3000;

    //Query payload constants
    const std::string OPERATOR_GRAPH_KEY = "operatorGraph";
    const std::string OPERATOR_ID_KEY = "operatorId";
    const std::string CHILDREN_KEY = "children";
    const std::string SOURCE_CODE_KEY = "sourceCode";
    const std::string CONSTRAINT_KEY = "constraint";
    const std::string INPUT_DATA_KEY = "inputData";

    //Topology payload constants
    const std::string DEVICE_ID_KEY = "deviceID";
    const std::string DEVICE_TYPE_KEY = "deviceType";
    const std::string DEVICE_NAME_KEY = "deviceName";
    const std::string MEMORY_KEY = "memory";
    const std::string DEVICES_KEY = "devices";
    const std::string NODE_ID_KEY = "nodeId";
    const std::string NODE_TYPE_KEY = "nodeType";

    //Network delay payload constants
    const std::string NETWORK_DELAYS_KEY = "networkDelays";
    const std::string LINK_ID_KEY = "linkID";
    const std::string TRANSFER_RATE_KEY = "transferRate";
    const std::string TIME_WEIGHT_KEY = "time_Weight";

    //Response payload constants
    const std::string AVAILABLE_NODES_KEY = "availNodes";
    const std::string PLACEMENT_KEY = "placement";
    const std::string OPTIMIZATION_OBJECTIVES_KEY = "optimizationObjectives";

    const std::string EMPTY_STRING;
};

}// namespace x::Optimizer

#endif// x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_ELEGANTPLACEMENTSTRATEGY_HPP_