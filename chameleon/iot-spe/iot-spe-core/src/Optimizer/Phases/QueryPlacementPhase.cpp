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

#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Exceptions/QueryPlacementException.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Optimizer/Phases/QueryPlacementPhase.hpp>
#include <Optimizer/QueryPlacement/ManualPlacementStrategy.hpp>
#include <Optimizer/QueryPlacement/PlacementStrategyFactory.hpp>
#include <Plans/ChangeLog/ChangeLogEntry.hpp>
#include <Plans/Global/Execution/GlobalExecutionPlan.hpp>
#include <Plans/Global/Query/SharedQueryPlan.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>
#include <algorithm>
#include <utility>

namespace x::Optimizer {

QueryPlacementPhase::QueryPlacementPhase(GlobalExecutionPlanPtr globalExecutionPlan,
                                         TopologyPtr topology,
                                         TypeInferencePhasePtr typeInferencePhase,
                                         Configurations::CoordinatorConfigurationPtr coordinatorConfiguration)
    : globalExecutionPlan(std::move(globalExecutionPlan)), topology(std::move(topology)),
      typeInferencePhase(std::move(typeInferencePhase)), coordinatorConfiguration(std::move(coordinatorConfiguration)) {
    x_DEBUG("QueryPlacementPhase()");
}

QueryPlacementPhasePtr QueryPlacementPhase::create(GlobalExecutionPlanPtr globalExecutionPlan,
                                                   TopologyPtr topology,
                                                   TypeInferencePhasePtr typeInferencePhase,
                                                   Configurations::CoordinatorConfigurationPtr coordinatorConfiguration) {
    return std::make_shared<QueryPlacementPhase>(QueryPlacementPhase(std::move(globalExecutionPlan),
                                                                     std::move(topology),
                                                                     std::move(typeInferencePhase),
                                                                     std::move(coordinatorConfiguration)));
}

bool QueryPlacementPhase::execute(const SharedQueryPlanPtr& sharedQueryPlan) {
    x_INFO("QueryPlacementPhase: Perform query placement phase for shared query plan {}",
             std::to_string(sharedQueryPlan->getId()));
    //TODO: At the time of placement we have to make sure that there are no changes done on xTopologyPlan (how to handle the case of dynamic topology?)
    // one solution could be: 1.) Take the snapshot of the topology and perform the placement 2.) If the topology changed meanwhile, repeat step 1.
    auto placementStrategy = sharedQueryPlan->getPlacementStrategy();
    auto placementStrategyPtr = PlacementStrategyFactory::getStrategy(placementStrategy,
                                                                      globalExecutionPlan,
                                                                      topology,
                                                                      typeInferencePhase,
                                                                      coordinatorConfiguration);

    bool queryReconfiguration = coordinatorConfiguration->enableQueryReconfiguration;

    auto sharedQueryId = sharedQueryPlan->getId();
    auto queryPlan = sharedQueryPlan->getQueryPlan();
    auto faultToleranceType = queryPlan->getFaultToleranceType();
    auto lineageType = queryPlan->getLineageType();
    x_DEBUG("QueryPlacementPhase: Perform query placement for query plan\n{}", queryPlan->toString());

    // Get current time stamp
    uint64_t nowInMicroSec =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (queryReconfiguration) {
        for (const auto& changeLogEntry : sharedQueryPlan->getChangeLogEntries(nowInMicroSec)) {

            //1. Fetch all upstream pinned operators
            auto pinnedUpstreamOperators = changeLogEntry.second->upstreamOperators;

            //2. Fetch all downstream pinned operators
            auto pinnedDownStreamOperators = changeLogEntry.second->downstreamOperators;

            //3. Pin all sink operators
            pinAllSinkOperators(pinnedDownStreamOperators);

            //4. Check if all operators are pinned
            if (!checkIfAllArePinnedOperators(pinnedDownStreamOperators)
                || !checkIfAllArePinnedOperators(pinnedUpstreamOperators)) {
                throw Exceptions::QueryPlacementException(sharedQueryId, "QueryPlacementPhase: Found operators without pinning.");
            }

            bool success = placementStrategyPtr->updateGlobalExecutionPlan(sharedQueryId,
                                                                           faultToleranceType,
                                                                           lineageType,
                                                                           pinnedUpstreamOperators,
                                                                           pinnedDownStreamOperators);

            if (!success) {
                x_ERROR("Unable to perform query placement for the change log entry");
                return false;
            }
        }
        //Update the change log's till processed timestamp and clear all entries before the timestamp
        sharedQueryPlan->updateProcessedChangeLogTimestamp(nowInMicroSec);
    } else {
        //1. Fetch all upstream pinned operators
        std::set<LogicalOperatorNodePtr> pinnedUpstreamOperators;
        for (const auto& leafOperator : queryPlan->getLeafOperators()) {
            pinnedUpstreamOperators.insert(leafOperator->as<LogicalOperatorNode>());
        };

        //2. Fetch all downstream pinned operators
        std::set<LogicalOperatorNodePtr> pinnedDownStreamOperators;
        for (const auto& rootOperator : queryPlan->getRootOperators()) {
            pinnedDownStreamOperators.insert(rootOperator->as<LogicalOperatorNode>());
        };

        //3. Pin all sink operators
        pinAllSinkOperators(pinnedDownStreamOperators);

        //4. Check if all operators are pinned
        if (!checkIfAllArePinnedOperators(pinnedDownStreamOperators) || !checkIfAllArePinnedOperators(pinnedUpstreamOperators)) {
            throw Exceptions::QueryPlacementException(sharedQueryId, "QueryPlacementPhase: Found operators without pinning.");
        }

        bool success = placementStrategyPtr->updateGlobalExecutionPlan(sharedQueryId,
                                                                       faultToleranceType,
                                                                       lineageType,
                                                                       pinnedUpstreamOperators,
                                                                       pinnedDownStreamOperators);

        if (!success) {
            x_ERROR("Unable to perform query placement for the change log entry");
            return false;
        }
    }

    x_DEBUG("QueryPlacementPhase: Update Global Execution Plan:\n{}", globalExecutionPlan->getAsString());
    return true;
}

bool QueryPlacementPhase::checkIfAllArePinnedOperators(const std::set<LogicalOperatorNodePtr>& pinnedOperators) {

    //Find if one of the operator does not have PINNED_NODE_ID property
    return !std::any_of(pinnedOperators.begin(), pinnedOperators.end(), [](const OperatorNodePtr& pinnedOperator) {
        return !pinnedOperator->hasProperty(PINNED_NODE_ID);
    });
}

void QueryPlacementPhase::pinAllSinkOperators(const std::set<LogicalOperatorNodePtr>& operators) {
    uint64_t rootNodeId = topology->getRoot()->getId();
    for (const auto& operatorToCheck : operators) {
        if (!operatorToCheck->hasProperty(PINNED_NODE_ID) && operatorToCheck->instanceOf<SinkLogicalOperatorNode>()) {
            operatorToCheck->addProperty(PINNED_NODE_ID, rootNodeId);
        }
    }
}
}// namespace x::Optimizer