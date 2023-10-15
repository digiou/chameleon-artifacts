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

#include <Nodes/Expressions/LogicalExpressions/AndExpressionNode.hpp>
#include <Nodes/Util/Iterators/DepthFirstNodeIterator.hpp>
#include <Operators/LogicalOperators/FilterLogicalOperatorNode.hpp>
#include <Optimizer/QueryRewrite/FilterMergeRule.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Optimizer {

FilterMergeRulePtr FilterMergeRule::create() { return std::make_shared<FilterMergeRule>(); }

QueryPlanPtr FilterMergeRule::apply(x::QueryPlanPtr queryPlan) {
    x_INFO("Applying FilterMergeRule to query {}", queryPlan->toString());
    std::set<NodeId> visitedNodesIds;
    auto filterOperators = queryPlan->getOperatorByType<FilterLogicalOperatorNode>();
    x_DEBUG("FilterMergeRule: Identified {} filter nodes in the query plan", filterOperators.size());
    x_DEBUG("Query before applying the rule: {}", queryPlan->toString());
    for (auto& filter : filterOperators) {
        if (visitedNodesIds.find(filter->getId()) == visitedNodesIds.end()) {
            std::vector<FilterLogicalOperatorNodePtr> consecutiveFilters = getConsecutiveFilters(filter);
            x_DEBUG("FilterMergeRule: Filter {} has {} consecutive filters as children",
                      filter->getId(),
                      consecutiveFilters.size());
            if (consecutiveFilters.size() >= 2) {
                x_DEBUG("FilterMerge: Create combined predicate");
                auto combinedPredicate = consecutiveFilters.at(0)->getPredicate();
                x_DEBUG("FilterMergeRule: The predicate of each consecutive filter should be appended to the conjunction");
                for (unsigned int i = 1; i < consecutiveFilters.size(); i++) {
                    auto predicate = consecutiveFilters.at(i)->getPredicate();
                    combinedPredicate = AndExpressionNode::create(combinedPredicate, predicate);
                }
                x_DEBUG("FilterMergeRule: Create new combined filter with the conjunction of all filter predicates");
                auto combinedFilter = LogicalOperatorFactory::createFilterOperator(combinedPredicate);
                auto filterChainParents = consecutiveFilters.at(0)->getParents();
                auto filterChainChildren = consecutiveFilters.back()->getChildren();
                x_DEBUG("FilterMergeRule: Start re-writing the new query plan");
                x_DEBUG("FilterMergeRule: Remove parent/children references for the consecutive filters");
                for (auto& filterToRemove : consecutiveFilters) {
                    filterToRemove->removeAllParent();
                    filterToRemove->removeChildren();
                }
                x_DEBUG("FilterMergeRule: Fix references, the parent of new filter are the parents of the filter chain");
                for (auto& filterChainParent : filterChainParents) {
                    combinedFilter->addParent(filterChainParent);
                }
                x_DEBUG(
                    "FilterMergeRule: Fix references, the chain children have only one parent, which is the new combined filter");
                for (auto& filterChainChild : filterChainChildren) {
                    filterChainChild->removeAllParent();
                    filterChainChild->addParent(combinedFilter);
                }
                x_DEBUG("FilterMergeRule: Mark the involved nodes as visited");
                for (auto& orderedFilter : consecutiveFilters) {
                    visitedNodesIds.insert(orderedFilter->getId());
                }
            } else {
                x_DEBUG("FilterMergeRule: Only one filter was found, no optimization is possible")
            }
        } else {
            x_DEBUG("FilterMergeRule: Filter node already visited");
        }
    }
    x_DEBUG("Query after applying the rule: {}", queryPlan->toString());
    return queryPlan;
}

std::vector<FilterLogicalOperatorNodePtr>
FilterMergeRule::getConsecutiveFilters(const x::FilterLogicalOperatorNodePtr& filter) {
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
