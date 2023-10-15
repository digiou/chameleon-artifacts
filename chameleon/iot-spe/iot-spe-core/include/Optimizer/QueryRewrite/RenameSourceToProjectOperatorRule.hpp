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

#ifndef x_CORE_INCLUDE_OPTIMIZER_QUERYREWRITE_RENAMESOURCETOPROJECTOPERATORRULE_HPP_
#define x_CORE_INCLUDE_OPTIMIZER_QUERYREWRITE_RENAMESOURCETOPROJECTOPERATORRULE_HPP_

#include <Optimizer/QueryRewrite/BaseRewriteRule.hpp>

namespace x::Optimizer {

class RenameSourceToProjectOperatorRule;
using RenameSourceToProjectOperatorRulePtr = std::shared_ptr<RenameSourceToProjectOperatorRule>;

/**
 * @brief This rule is responsible for transforming Source Rename operator to the projection operator
 */
class RenameSourceToProjectOperatorRule : public BaseRewriteRule {

  public:
    QueryPlanPtr apply(QueryPlanPtr queryPlan) override;
    virtual ~RenameSourceToProjectOperatorRule() = default;

    static RenameSourceToProjectOperatorRulePtr create();

  private:
    RenameSourceToProjectOperatorRule() = default;

    /**
     * @brief Convert input operator into project operator
     * @param operatorNode : the rename source operator
     * @return pointer to the converted project operator
     */
    static OperatorNodePtr convert(const OperatorNodePtr& operatorNode);
};

}// namespace x::Optimizer

#endif// x_CORE_INCLUDE_OPTIMIZER_QUERYREWRITE_RENAMESOURCETOPROJECTOPERATORRULE_HPP_
