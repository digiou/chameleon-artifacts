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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_UNIONLOGICALOPERATORNODE_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_UNIONLOGICALOPERATORNODE_HPP_

#include <Operators/LogicalOperators/LogicalBinaryOperatorNode.hpp>
#include <Operators/OperatorForwardDeclaration.hpp>

namespace x {

/**
 * @brief Union operator that Union two sources together. This operator behaves similar to the unionWith operator in RDBMS.
 */
class UnionLogicalOperatorNode : public LogicalBinaryOperatorNode {
  public:
    explicit UnionLogicalOperatorNode(OperatorId id);
    ~UnionLogicalOperatorNode() override = default;

    [[nodiscard]] bool isIdentical(NodePtr const& rhs) const override;
    std::string toString() const override;
    //infer schema of two child operators
    bool inferSchema(Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext) override;
    void inferInputOrigins() override;
    void inferStringSignature() override;
    OperatorNodePtr copy() override;
    [[nodiscard]] bool equal(NodePtr const& rhs) const override;
};
}// namespace x
#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_UNIONLOGICALOPERATORNODE_HPP_
