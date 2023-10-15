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
#include <Exceptions/RuntimeException.hpp>
#include <Optimizer/QueryPlacement/BottomUpStrategy.hpp>
#include <Optimizer/QueryPlacement/ElegantPlacementStrategy.hpp>
#include <Optimizer/QueryPlacement/IFCOPStrategy.hpp>
#include <Optimizer/QueryPlacement/ILPStrategy.hpp>
#include <Optimizer/QueryPlacement/ManualPlacementStrategy.hpp>
#include <Optimizer/QueryPlacement/MlHeuristicStrategy.hpp>
#include <Optimizer/QueryPlacement/PlacementStrategyFactory.hpp>
#include <Optimizer/QueryPlacement/TopDownStrategy.hpp>
#include <Util/PlacementStrategy.hpp>
#include <Util/magicenum/magic_enum.hpp>

namespace x::Optimizer {

BasePlacementStrategyPtr
PlacementStrategyFactory::getStrategy(PlacementStrategy placementStrategy,
                                      const GlobalExecutionPlanPtr& globalExecutionPlan,
                                      const TopologyPtr& topology,
                                      const TypeInferencePhasePtr& typeInferencePhase,
                                      const Configurations::CoordinatorConfigurationPtr& coordinatorConfiguration) {

    std::string plannerURL = coordinatorConfiguration->elegantConfiguration.plannerServiceURL;

    switch (placementStrategy) {
        case PlacementStrategy::ILP: return ILPStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
        case PlacementStrategy::BottomUp: return BottomUpStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
        case PlacementStrategy::TopDown: return TopDownStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
        case PlacementStrategy::Manual: return ManualPlacementStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
        case PlacementStrategy::ELEGANT_PERFORMANCE:
        case PlacementStrategy::ELEGANT_ENERGY:
        case PlacementStrategy::ELEGANT_BALANCED:
            return ElegantPlacementStrategy::create(plannerURL,
                                                    placementStrategy,
                                                    globalExecutionPlan,
                                                    topology,
                                                    typeInferencePhase);

// #2486        case PlacementStrategy::IFCOP:
//            return IFCOPStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
#ifdef TFDEF
        case PlacementStrategy::MlHeuristic:
            return MlHeuristicStrategy::create(globalExecutionPlan, topology, typeInferencePhase);
#endif

        // FIXME: enable them with issue #755
        //        case LowLatency: return LowLatencyStrategy::create(xTopologyPlan);
        //        case HighThroughput: return HighThroughputStrategy::create(xTopologyPlan);
        //        case MinimumResourceConsumption: return MinimumResourceConsumptionStrategy::create(xTopologyPlan);
        //        case MinimumEnergyConsumption: return MinimumEnergyConsumptionStrategy::create(xTopologyPlan);
        //        case HighAvailability: return HighAvailabilityStrategy::create(xTopologyPlan);
        default:
            throw Exceptions::RuntimeException("Unknown placement strategy type "
                                               + std::string(magic_enum::enum_name(placementStrategy)));
    }
}
}// namespace x::Optimizer