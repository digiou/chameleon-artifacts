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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_WINDOWING_SLICECREATIONOPERATOR_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_WINDOWING_SLICECREATIONOPERATOR_HPP_
#include <Operators/LogicalOperators/LogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Windowing/WindowLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Windowing/WindowOperatorNode.hpp>

namespace x {

/**
 * @brief this class represents the slicing operator for distributed windowing that is deployed on the source nodes and send all sliches to the combiner
 */
class SliceCreationOperator : public WindowOperatorNode {
  public:
    SliceCreationOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition, OperatorId id);

    [[nodiscard]] bool equal(NodePtr const& rhs) const override;
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool isIdentical(NodePtr const& rhs) const override;
    OperatorNodePtr copy() override;
    bool inferSchema(Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext) override;
    void inferStringSignature() override;
};

}// namespace x
#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_WINDOWING_SLICECREATIONOPERATOR_HPP_
