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

#ifndef x_FILTERSPLITUPRULE_HPP
#define x_FILTERSPLITUPRULE_HPP

#include <Optimizer/QueryRewrite/BaseRewriteRule.hpp>

namespace x {

class Node;
using NodePtr = std::shared_ptr<Node>;

class FilterLogicalOperatorNode;
using FilterLogicalOperatorNodePtr = std::shared_ptr<FilterLogicalOperatorNode>;
}// namespace x

namespace x::Optimizer {

class FilterSplitUpRule;
using FilterSplitUpRulePtr = std::shared_ptr<FilterSplitUpRule>;

/**
 * @brief This class is responsible for altering the query plan to split up each filter operator into as small parts as possible.
 *  1.) A filter with an andExpression as a predicate can be split up into two separate filters.
 *  2.) A filter with a negated OrExpression can be reformulated deMorgans rules and can be split up afterwards.
 */
class FilterSplitUpRule : public BaseRewriteRule {
  public:
    QueryPlanPtr apply(QueryPlanPtr queryPlan) override;

    static FilterSplitUpRulePtr create();
    virtual ~FilterSplitUpRule() = default;

  private:
    explicit FilterSplitUpRule();

    /**
     * If it is possible this method splits up a filterOperator into multiple filterOperators.
     * If our query plan contains a parentOperaters->filter(expression1 && expression2)->childOperator.
     * This plan gets rewritten to parentOperaters->filter(expression1)->filter(expression2)->childOperator. We will call splitUpFilters()
     * on the new flters as well
     * If our query plan contains a parentOperaters->filter(!(expression1 || expression2))->childOperator, we use deMorgan to
     * reformulate the predicate to an andExpression and call splitUpFilter on the Filter.
     * @param filterOperator the filter operator node that we want to split up
     */
    void splitUpFilters(FilterLogicalOperatorNodePtr filterOperator);
};

}// namespace x::Optimizer

#endif//x_FILTERSPLITUPRULE_HPP
