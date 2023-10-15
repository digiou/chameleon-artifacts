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
#include <Operators/LogicalOperators/FilterLogicalOperatorNode.hpp>
#include <Optimizer/QueryRewrite/PredicateReorderingRule.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Optimizer {

PredicateReorderingRulePtr PredicateReorderingRule::create() { return std::make_shared<PredicateReorderingRule>(); }

QueryPlanPtr PredicateReorderingRule::apply(x::QueryPlanPtr queryPlan) {
    std::set<NodeId> visitedNodesIds;
    auto filterOperators = queryPlan->getOperatorByType<FilterLogicalOperatorNode>();
    x_DEBUG("PredicateReorderingRule: Identified {} filter nodes in the query plan", filterOperators.size());
    x_DEBUG("Query before applying the rule: {}", queryPlan->toString());
    for (auto& filter : filterOperators) {
        if (visitedNodesIds.find(filter->getId()) == visitedNodesIds.end()) {
            std::vector<FilterLogicalOperatorNodePtr> consecutiveFilters = getConsecutiveFilters(filter);
            x_TRACE("PredicateReorderingRule: Filter {} has {} consecutive filters as children",
                      filter->getId(),
                      consecutiveFilters.size());
            if (consecutiveFilters.size() >= 2) {
                std::vector<NodePtr> filterChainParents = consecutiveFilters.front()->getParents();
                std::vector<NodePtr> filterChainChildren = consecutiveFilters.back()->getChildren();
                x_TRACE("PredicateReorderingRule: If the filters are already sorted, no change is needed");
                auto already_sorted =
                    std::is_sorted(consecutiveFilters.begin(),
                                   consecutiveFilters.end(),
                                   [](const FilterLogicalOperatorNodePtr& lhs, const FilterLogicalOperatorNodePtr& rhs) {
                                       return lhs->getSelectivity() < rhs->getSelectivity();
                                   });
                if (!already_sorted) {
                    x_TRACE("PredicateReorderingRule: Sort all filter nodes in increasing order of selectivity");
                    std::sort(consecutiveFilters.begin(),
                              consecutiveFilters.end(),
                              [](const FilterLogicalOperatorNodePtr& lhs, const FilterLogicalOperatorNodePtr& rhs) {
                                  return lhs->getSelectivity() < rhs->getSelectivity();
                              });
                    x_TRACE("PredicateReorderingRule: Start re-writing the new query plan");
                    x_TRACE("PredicateReorderingRule: Remove parent/children references");
                    for (auto& filterToReassign : consecutiveFilters) {
                        filterToReassign->removeAllParent();
                        filterToReassign->removeChildren();
                    }
                    x_TRACE("PredicateReorderingRule: For each filter, reassign children according to new order");
                    for (unsigned int i = 0; i < consecutiveFilters.size() - 1; i++) {
                        auto filterToReassign = consecutiveFilters.at(i);
                        filterToReassign->addChild(consecutiveFilters.at(i + 1));
                    }
                    x_TRACE("PredicateReorderingRule: Retrieve most and least selective filters");
                    auto leastSelectiveFilter = consecutiveFilters.front();
                    auto mostSelectiveFilter = consecutiveFilters.back();
                    x_TRACE("PredicateReorderingRule: Least selective filter goes to the top (closer to sink), his new "
                              "parents will be the chain parents'");
                    for (auto& filterChainParent : filterChainParents) {
                        leastSelectiveFilter->addParent(filterChainParent);
                    }
                    x_TRACE("PredicateReorderingRule: Most selective filter goes to the bottom (closer to the source), his "
                              "new children will be the chain children");
                    for (auto& filterChainChild : filterChainChildren) {
                        mostSelectiveFilter->addChild(filterChainChild);
                    }
                }
                x_TRACE("PredicateReorderingRule: Mark the involved nodes as visited");
                for (auto& orderedFilter : consecutiveFilters) {
                    visitedNodesIds.insert(orderedFilter->getId());
                }
            }
        } else {
            x_TRACE("PredicateReorderingRule: Filter node already visited");
        }
    }
    x_DEBUG("Query after applying the rule: {}", queryPlan->toString());
    return queryPlan;
}

std::vector<FilterLogicalOperatorNodePtr>
PredicateReorderingRule::getConsecutiveFilters(const x::FilterLogicalOperatorNodePtr& filter) {
    std::vector<FilterLogicalOperatorNodePtr> consecutiveFilters = {};
    DepthFirstNodeIterator queryPlanNodeIterator(filter);
    auto nodeIterator = queryPlanNodeIterator.begin();
    auto node = (*nodeIterator);
    while (node->instanceOf<FilterLogicalOperatorNode>()) {
        x_DEBUG("Found consecutive filter in the chain, adding it the list");
        consecutiveFilters.push_back(node->as<FilterLogicalOperatorNode>());
        ++nodeIterator;
        node = (*nodeIterator);
    }
    x_DEBUG("Found {} consecutive filters", consecutiveFilters.size());
    return consecutiveFilters;
}

}// namespace x::Optimizer
