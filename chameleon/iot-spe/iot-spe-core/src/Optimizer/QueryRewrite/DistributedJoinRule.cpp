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
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Operators/LogicalOperators/JoinLogicalOperatorNode.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QueryRewrite/DistributeJoinRule.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalJoinDefinition.hpp>

namespace x::Optimizer {

DistributeJoinRule::DistributeJoinRule() = default;

DistributeJoinRulePtr DistributeJoinRule::create() { return std::make_shared<DistributeJoinRule>(DistributeJoinRule()); }

QueryPlanPtr DistributeJoinRule::apply(QueryPlanPtr queryPlan) {
    x_INFO("DistributeJoinRule: Apply DistributeJoinRule.");
    x_DEBUG("DistributeJoinRule::apply: plan before replace\n{}", queryPlan->toString());
    auto joinOps = queryPlan->getOperatorByType<JoinLogicalOperatorNode>();
    if (!joinOps.empty()) {
        x_DEBUG("DistributeJoinRule::apply: found {} join operators", joinOps.size());
        for (auto& joinOp : joinOps) {
            x_DEBUG("DistributeJoinRule::apply: join operator {}", joinOp->toString());
            auto leftInputSchema = joinOp->getLeftInputSchema();
            uint64_t edgesLeft = 0;
            uint64_t edgesRight = 0;
            auto children = joinOp->getChildren();
            for (const auto& child : children) {
                auto childOperator = child->as<OperatorNode>();
                if (childOperator->getOutputSchema()->equals(leftInputSchema, false)) {
                    edgesLeft++;
                } else {
                    edgesRight++;
                }
            }
            x_DEBUG("DistributeJoinRule set edgesLeft={} edgesRight={}", edgesLeft, edgesRight);
            joinOp->getJoinDefinition()->setNumberOfInputEdgesLeft(edgesLeft);
            joinOp->getJoinDefinition()->setNumberOfInputEdgesRight(edgesRight);
        }
    } else {
        x_DEBUG("DistributeJoinRule::apply: no join operator in query");
    }

    x_DEBUG("DistributeJoinRule::apply: plan after replace\n{}", queryPlan->toString());

    return queryPlan;
}

}// namespace x::Optimizer
