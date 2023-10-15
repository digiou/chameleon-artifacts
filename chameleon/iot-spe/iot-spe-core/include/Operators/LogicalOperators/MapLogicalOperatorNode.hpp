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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_MAPLOGICALOPERATORNODE_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_MAPLOGICALOPERATORNODE_HPP_

#include <Operators/LogicalOperators/LogicalUnaryOperatorNode.hpp>
#include <Operators/OperatorForwardDeclaration.hpp>

namespace x {

/**
 * @brief Map operator, which contains an field assignment expression that manipulates a field of the record.
 */
class MapLogicalOperatorNode : public LogicalUnaryOperatorNode {
  public:
    MapLogicalOperatorNode(FieldAssignmentExpressionNodePtr const& mapExpression, OperatorId id);

    /**
    * @brief Returns the expression of this map operator
    * @return FieldAssignmentExpressionNodePtr
    */
    FieldAssignmentExpressionNodePtr getMapExpression() const;

    /**
     * @brief Infers the schema of the map operator. We support two cases:
     * 1. the assignment statement manipulates a already existing field. In this case the data type of the field can change.
     * 2. the assignment statement creates a new field with an inferred data type.
     * @throws throws exception if inference was not possible.
     * @param typeInferencePhaseContext needed for stamp inferring
     * @return true if inference was possible
     */
    bool inferSchema(Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext) override;
    void inferStringSignature() override;
    [[nodiscard]] bool equal(NodePtr const& rhs) const override;
    [[nodiscard]] bool isIdentical(NodePtr const& rhs) const override;
    std::string toString() const override;
    OperatorNodePtr copy() override;

  private:
    const FieldAssignmentExpressionNodePtr mapExpression;
};

}// namespace x

#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_MAPLOGICALOPERATORNODE_HPP_
