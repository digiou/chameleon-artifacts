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

#ifndef x_CORE_INCLUDE_NODES_EXPRESSIONS_CONSTANTVALUEEXPRESSIONNODE_HPP_
#define x_CORE_INCLUDE_NODES_EXPRESSIONS_CONSTANTVALUEEXPRESSIONNODE_HPP_

#include <Nodes/Expressions/ExpressionNode.hpp>
namespace x {

class ValueType;
using ValueTypePtr = std::shared_ptr<ValueType>;

/**
 * @brief This expression node represents a constant value and a fixed data type.
 * Thus the samp of this expression is always fixed.
 */
class ConstantValueExpressionNode : public ExpressionNode {
  public:
    /**
     * @brief Factory method to create a ConstantValueExpressionNode.
     */
    static ExpressionNodePtr create(ValueTypePtr const& constantValue);
    virtual ~ConstantValueExpressionNode() noexcept = default;

    /**
     * @brief Returns the constant value.
     */
    ValueTypePtr getConstantValue() const;

    /**
     * @brief On a constant value expression infer stamp has not to perform any action as its result type is always constant.
     * @param typeInferencePhaseContext
     * @param schema
     */
    void inferStamp(const Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext, SchemaPtr schema) override;

    /**
     * @brief Creates a string of the value and the type.
     * @return
     */
    std::string toString() const override;

    /**
     * @brief Compares if another node is equal to this constant value expression.
     * @param otherNode
     * @return true if they are equal
     */
    bool equal(NodePtr const& rhs) const override;

    /**
    * @brief Create a deep copy of this expression node.
    * @return ExpressionNodePtr
    */
    ExpressionNodePtr copy() override;

  protected:
    explicit ConstantValueExpressionNode(ConstantValueExpressionNode* other);

  private:
    explicit ConstantValueExpressionNode(ValueTypePtr const& constantValue);
    // Value of this expression
    ValueTypePtr constantValue;
};

}// namespace x

#endif// x_CORE_INCLUDE_NODES_EXPRESSIONS_CONSTANTVALUEEXPRESSIONNODE_HPP_
