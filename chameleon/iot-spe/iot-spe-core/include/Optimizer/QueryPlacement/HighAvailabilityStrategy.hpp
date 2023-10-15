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

#ifndef x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_HIGHAVAILABILITYSTRATEGY_HPP_
#define x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_HIGHAVAILABILITYSTRATEGY_HPP_

#include <Optimizer/QueryPlacement/BasePlacementStrategy.hpp>

namespace x {

/**
 * @brief This Class is responsible for placing operators on the nodes such that there exists R number of redundant
 * paths between the operator and the source node.
 */
class HighAvailabilityStrategy : public BasePlacementStrategy {

  public:
    ~HighAvailabilityStrategy() = default;
    GlobalExecutionPlanPtr initializeExecutionPlan(QueryPtr inputQuery, Catalogs::Source::SourceCatalogPtr sourceCatalog);

    static std::unique_ptr<HighAvailabilityStrategy> create(xTopologyPlanPtr xTopologyPlan) {
        return std::make_unique<HighAvailabilityStrategy>(HighAvailabilityStrategy(xTopologyPlan));
    }

  private:
    HighAvailabilityStrategy(xTopologyPlanPtr xTopologyPlan);

    /**
     * This method is responsible for placing the operators to the x nodes and generating ExecutionNodes.
     * @param xExecutionPlanPtr : graph containing the information about the execution nodes.
     * @param xTopologyGraphPtr : x Topology graph used for extracting information about the x topology.
     * @param sourceNodePtr : sensor nodes which can act as source.
     *
     * @throws exception if the operator can't be placed anywhere.
     */
    void placeOperators(xExecutionPlanPtr xExecutionPlanPtr,
                        xTopologyGraphPtr xTopologyGraphPtr,
                        LogicalOperatorNodePtr sourceOperator,
                        std::vector<xTopologyEntryPtr> sourceNodes);

    /**
     * @brief Add forward operators between source and sink nodes.
     * @param rootNode : sink node
     * @param xExecutionPlanPtr : x execution plan
     */
    void addForwardOperators(std::vector<xTopologyEntryPtr> pathForPlacement, xExecutionPlanPtr xExecutionPlanPtr) const;

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
#endif// x_CORE_INCLUDE_OPTIMIZER_QUERYPLACEMENT_HIGHAVAILABILITYSTRATEGY_HPP_
