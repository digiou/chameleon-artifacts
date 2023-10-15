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

#ifndef x_CORE_INCLUDE_NODES_EXPRESSIONS_ARITHMETICALEXPRESSIONS_CEILEXPRESSIONNODE_HPP_
#define x_CORE_INCLUDE_NODES_EXPRESSIONS_ARITHMETICALEXPRESSIONS_CEILEXPRESSIONNODE_HPP_
#include <Nodes/Expressions/ArithmeticalExpressions/ArithmeticalUnaryExpressionNode.hpp>
namespace x {
/**
 * @brief This node represents an CEIL (ceiling of) expression.
 */
class CeilExpressionNode final : public ArithmeticalUnaryExpressionNode {
  public:
    explicit CeilExpressionNode(DataTypePtr stamp);
    ~CeilExpressionNode() noexcept final = default;
    /**
     * @brief Create a new CEIL expression
     */
    [[nodiscard]] static ExpressionNodePtr create(ExpressionNodePtr const& child);
    [[nodiscard]] bool equal(NodePtr const& rhs) const final;
    [[nodiscard]] std::string toString() const final;

    /**
     * @brief Infers the stamp of the expression given the current schema and the typeInferencePhaseContext.
     * @param typeInferencePhaseContext
     * @param schema
     */
    void inferStamp(const Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext, SchemaPtr schema) final;

    /**
    * @brief Create a deep copy of this expression node.
    * @return ExpressionNodePtr
    */
    ExpressionNodePtr copy() final;

  protected:
    explicit CeilExpressionNode(CeilExpressionNode* other);
};

}// namespace x

#endif// x_CORE_INCLUDE_NODES_EXPRESSIONS_ARITHMETICALEXPRESSIONS_CEILEXPRESSIONNODE_HPP_