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

#ifndef x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_TOPDOWNSTRATEGY_HPP_
#define x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_TOPDOWNSTRATEGY_HPP_

#include <Optimizer/QueryPlacement/BasePlacementStrategy.hpp>
#include <stack>

namespace x::Optimizer {

class TopDownStrategy : public BasePlacementStrategy {

  public:
    ~TopDownStrategy() override = default;

    static BasePlacementStrategyPtr
    create(GlobalExecutionPlanPtr globalExecutionPlan, TopologyPtr topology, TypeInferencePhasePtr typeInferencePhase);

    bool updateGlobalExecutionPlan(QueryId queryId,
                                   FaultToleranceType faultToleranceType,
                                   LineageType lineageType,
                                   const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                                   const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators) override;

  private:
    TopDownStrategy(GlobalExecutionPlanPtr globalExecutionPlan, TopologyPtr topology, TypeInferencePhasePtr typeInferencePhase);

    /**
     * @brief place query operators and prepare the global execution plan
     * @param queryPlan: query plan to place
     * @param pinnedUpStreamOperators: pinned upstream operators
     * @param pinnedDownStreamOperators: pinned downstream operators
     * @throws exception if the operator can't be placed.
     */
    void pinOperators(QueryId queryId,
                      const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators,
                      const std::set<LogicalOperatorNodePtr>& pinnedDownStreamOperators);

    /**
     * @brief Try to place input operator on the input topology node
     * @param pinnedUpStreamOperator :  the query id
     * @param logicalOperator : the input operator to place
     * @param candidateTopologyNode : the candidate topology node to place operator on
     */
    void identifyPinningLocation(QueryId pinnedUpStreamOperator,
                                 const LogicalOperatorNodePtr& logicalOperator,
                                 TopologyNodePtr candidateTopologyNode,
                                 const std::set<LogicalOperatorNodePtr>& pinnedUpStreamOperators);

    /**
     * @brief Get topology node where all parent operators of the input operator are placed
     * @param candidateOperator: the input operator
     * @return vector of topology nodes where parent operator was placed or empty if not all parent operators are placed
     */
    std::vector<TopologyNodePtr> getTopologyNodesForDownStreamOperators(const LogicalOperatorNodePtr& candidateOperator);

    /**
     * @brief Get topology node where all children operators of the input operator are to be placed
     * @param candidateOperator: the input operator
     * @return vector of topology nodes where child operator are to be placed
     */
    std::vector<TopologyNodePtr> getTopologyNodesForUpStreamOperators(const LogicalOperatorNodePtr& candidateOperator);
};

}// namespace x::Optimizer
#endif// x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_TOPDOWNSTRATEGY_HPP_
