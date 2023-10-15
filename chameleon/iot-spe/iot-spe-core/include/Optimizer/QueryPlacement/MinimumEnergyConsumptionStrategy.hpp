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

#ifndef x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_MINIMUMENERGYCONSUMPTIONSTRATEGY_HPP_
#define x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_MINIMUMENERGYCONSUMPTIONSTRATEGY_HPP_

#include <Optimizer/QueryPlacement/BasePlacementStrategy.hpp>

namespace x {

/**
 * @brief This class is responsible for placing the operators on common path among the sources such that overall
 * energy consumption will reduce.
 */
class MinimumEnergyConsumptionStrategy : public BasePlacementStrategy {

  public:
    ~MinimumEnergyConsumptionStrategy() = default;
    GlobalExecutionPlanPtr initializeExecutionPlan(QueryPtr inputQuery, Catalogs::Source::SourceCatalogPtr sourceCatalog);

    static std::unique_ptr<MinimumEnergyConsumptionStrategy> create(xTopologyPlanPtr xTopologyPlan) {
        return std::make_unique<MinimumEnergyConsumptionStrategy>(MinimumEnergyConsumptionStrategy(xTopologyPlan));
    }

  private:
    MinimumEnergyConsumptionStrategy(xTopologyPlanPtr xTopologyPlan);

    void placeOperators(xExecutionPlanPtr executionPlanPtr,
                        xTopologyGraphPtr xTopologyGraphPtr,
                        LogicalOperatorNodePtr sourceOperator,
                        std::vector<xTopologyEntryPtr> sourceNodes);
    /**
     * @brief Finds all the nodes that can be used for performing FWD operator
     * @param sourceNodes
     * @param rootNode
     * @return
     */
    std::vector<xTopologyEntryPtr> getCandidateNodesForFwdOperatorPlacement(const std::vector<xTopologyEntryPtr>& sourceNodes,
                                                                              const xTopologyEntryPtr rootNode) const;
};
}// namespace x

#endif// x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_MINIMUMENERGYCONSUMPTIONSTRATEGY_HPP_
