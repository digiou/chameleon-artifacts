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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_RENAMESOURCEOPERATORNODE_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_RENAMESOURCEOPERATORNODE_HPP_

#include <Operators/LogicalOperators/LogicalUnaryOperatorNode.hpp>
#include <Operators/OperatorForwardDeclaration.hpp>

namespace x {

/**
 * @brief this operator renames the source
 */
class RenameSourceOperatorNode : public LogicalUnaryOperatorNode {
  public:
    explicit RenameSourceOperatorNode(std::string const& newSourceName, OperatorId id);
    ~RenameSourceOperatorNode() override = default;

    /**
     * @brief check if two operators have the same output schema
     * @param rhs the operator to compare
     * @return bool true if they are the same otherwise false
     */
    [[nodiscard]] bool equal(NodePtr const& rhs) const override;
    [[nodiscard]] bool isIdentical(NodePtr const& rhs) const override;
    [[nodiscard]] std::string toString() const override;

    /**
    * @brief infers the input and out schema of this operator depending on its child.
    * @param typeInferencePhaseContext needed for stamp inferring
    * @return true if schema was correctly inferred
    */
    bool inferSchema(Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext) override;
    OperatorNodePtr copy() override;
    void inferStringSignature() override;
    std::string getNewSourceName();

  private:
    const std::string newSourceName;
};

}// namespace x
#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_RENAMESOURCEOPERATORNODE_HPP_
