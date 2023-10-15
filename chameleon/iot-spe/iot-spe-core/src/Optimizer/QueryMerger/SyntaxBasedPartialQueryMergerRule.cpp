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
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Operators/LogicalOperators/JoinLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/WatermarkAssignerLogicalOperatorNode.hpp>
#include <Operators/OperatorNode.hpp>
#include <Optimizer/QueryMerger/MatchedOperatorPair.hpp>
#include <Optimizer/QueryMerger/SyntaxBasedPartialQueryMergerRule.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Plans/Global/Query/SharedQueryPlan.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/LogicalJoinDefinition.hpp>

namespace x::Optimizer {

SyntaxBasedPartialQueryMergerRulePtr SyntaxBasedPartialQueryMergerRule::create() {
    return std::make_shared<SyntaxBasedPartialQueryMergerRule>();
}

bool SyntaxBasedPartialQueryMergerRule::apply(GlobalQueryPlanPtr globalQueryPlan) {

    x_INFO("SyntaxBasedPartialQueryMergerRule: Applying Syntax Based Equal Query Merger Rule to the Global Query Plan");
    std::vector<QueryPlanPtr> queryPlansToAdd = globalQueryPlan->getQueryPlansToAdd();
    if (queryPlansToAdd.empty()) {
        x_WARNING("SyntaxBasedPartialQueryMergerRule: Found no new query metadata in the global query plan."
                    " Skipping the Syntax Based Equal Query Merger Rule.");
        return true;
    }

    x_DEBUG("SyntaxBasedPartialQueryMergerRule: Iterating over all GQMs in the Global Query Plan");
    for (auto& targetQueryPlan : queryPlansToAdd) {
        bool merged = false;
        auto hostSharedQueryPlans =
            globalQueryPlan->getSharedQueryPlansConsumingSourcesAndPlacementStrategy(targetQueryPlan->getSourceConsumed(),
                                                                                     targetQueryPlan->getPlacementStrategy());
        for (auto& hostSharedQueryPlan : hostSharedQueryPlans) {

            //TODO: we need to check how this will pan out when we will have more than 1 sink
            auto hostQueryPlan = hostSharedQueryPlan->getQueryPlan();

            //create a map of matching target to address operator id map
            auto matchedTargetToHostOperatorMap = areQueryPlansEqual(targetQueryPlan, hostQueryPlan);

            //Check if the target and address query plan are equal and return the target and address operator mappings
            if (!matchedTargetToHostOperatorMap.empty()) {
                x_TRACE("SyntaxBasedPartialQueryMergerRule: Merge target Shared metadata into address metadata");

                // As we merge partially equivalent queryIdAndCatalogEntryMapping, we can potentially find matches across multiple operators.
                // As upstream matched operators are covered by downstream matched operators. We need to retain only the
                // downstream matched operator containing any upstream matched operator. This will prevent in computation
                // of inconsistent shared query plans.
                if (matchedTargetToHostOperatorMap.size() > 1) {
                    //Fetch all the matched target operators.
                    std::vector<OperatorNodePtr> matchedTargetOperators;
                    matchedTargetOperators.reserve(matchedTargetToHostOperatorMap.size());
                    for (auto& mapEntry : matchedTargetToHostOperatorMap) {
                        matchedTargetOperators.emplace_back(mapEntry.first);
                    }

                    //Iterate over the target operators and remove the upstream operators covered by downstream matched operators
                    for (uint64_t i = 0; i < matchedTargetOperators.size(); i++) {
                        for (uint64_t j = 0; j < matchedTargetOperators.size(); j++) {
                            if (i == j) {
                                continue;//Skip chk with itself
                            }

                            if (matchedTargetOperators[i]->containAsGrandChild(matchedTargetOperators[j])) {
                                matchedTargetToHostOperatorMap.erase(matchedTargetOperators[j]);
                            } else if (matchedTargetOperators[i]->containAsGrandParent(matchedTargetOperators[j])) {
                                matchedTargetToHostOperatorMap.erase(matchedTargetOperators[i]);
                                break;
                            }
                        }
                    }
                }

                std::vector<MatchedOperatorPairPtr> matchedOperatorPairs;
                matchedOperatorPairs.reserve(matchedTargetToHostOperatorMap.size());
                //Iterate over all matched pairs of operators and merge the query plan
                for (auto [targetOperator, hostOperator] : matchedTargetToHostOperatorMap) {
                    matchedOperatorPairs.emplace_back(MatchedOperatorPair::create(hostOperator->as<LogicalOperatorNode>(),
                                                                                  targetOperator->as<LogicalOperatorNode>(),
                                                                                  ContainmentRelationship::EQUALITY));
                }

                //add matched operators to the host shared query plan
                hostSharedQueryPlan->addQuery(targetQueryPlan->getQueryId(), matchedOperatorPairs);

                //Update the shared query meta data
                globalQueryPlan->updateSharedQueryPlan(hostSharedQueryPlan);
                // exit the for loop as we found a matching address shared query meta data
                merged = true;
                break;
            }
        }
        if (!merged) {
            x_DEBUG("SyntaxBasedPartialQueryMergerRule: computing a new Shared Query Plan");
            globalQueryPlan->createNewSharedQueryPlan(targetQueryPlan);
        }
    }
    //Remove all empty shared query metadata
    globalQueryPlan->removeFailedOrStoppedSharedQueryPlans();
    return globalQueryPlan->clearQueryPlansToAdd();
}

std::map<OperatorNodePtr, OperatorNodePtr>
SyntaxBasedPartialQueryMergerRule::areQueryPlansEqual(const QueryPlanPtr& targetQueryPlan, const QueryPlanPtr& hostQueryPlan) {

    std::map<OperatorNodePtr, OperatorNodePtr> targetHostOperatorMap;
    x_DEBUG("SyntaxBasedPartialQueryMergerRule: check if the target and address query plans are syntactically equal or not");
    std::vector<OperatorNodePtr> targetSourceOperators = targetQueryPlan->getLeafOperators();
    std::vector<OperatorNodePtr> hostSourceOperators = hostQueryPlan->getLeafOperators();

    if (targetSourceOperators.size() != hostSourceOperators.size()) {
        x_WARNING("SyntaxBasedPartialQueryMergerRule: Not matched as number of sink in target and host query plans are "
                    "different.");
        return {};
    }

    //Fetch the first source operator and find a corresponding matching source operator in the address source operator list
    for (auto& targetSourceOperator : targetSourceOperators) {
        for (auto& hostSourceOperator : hostSourceOperators) {
            auto matchedOperators = areOperatorEqual(targetSourceOperator, hostSourceOperator);
            if (!matchedOperators.empty()) {
                targetHostOperatorMap.merge(matchedOperators);
                break;
            }
        }
    }
    return targetHostOperatorMap;
}

std::map<OperatorNodePtr, OperatorNodePtr>
SyntaxBasedPartialQueryMergerRule::areOperatorEqual(const OperatorNodePtr& targetOperator, const OperatorNodePtr& hostOperator) {

    std::map<OperatorNodePtr, OperatorNodePtr> targetHostOperatorMap;
    if (targetOperator->instanceOf<SinkLogicalOperatorNode>() && hostOperator->instanceOf<SinkLogicalOperatorNode>()) {
        x_TRACE("SyntaxBasedPartialQueryMergerRule: Both target and host operators are of sink type.");
        return {};
    }

    x_TRACE("SyntaxBasedPartialQueryMergerRule: Compare target and host operators.");
    if (targetOperator->equal(hostOperator)) {
        x_TRACE("SyntaxBasedPartialQueryMergerRule: Check if parents of target and address operators are equal.");
        uint16_t matchCount = 0;
        for (const auto& targetParent : targetOperator->getParents()) {
            for (const auto& hostParent : hostOperator->getParents()) {
                auto matchedOperators = areOperatorEqual(targetParent->as<OperatorNode>(), hostParent->as<OperatorNode>());
                if (!matchedOperators.empty()) {
                    targetHostOperatorMap.merge(matchedOperators);
                    matchCount++;
                    break;
                }
            }
        }

        if (matchCount < targetOperator->getParents().size()) {
            targetHostOperatorMap[targetOperator] = hostOperator;
        }
        return targetHostOperatorMap;
    }
    x_WARNING("SyntaxBasedPartialQueryMergerRule: Target and host operators are not matched.");
    return {};
}
}// namespace x::Optimizer