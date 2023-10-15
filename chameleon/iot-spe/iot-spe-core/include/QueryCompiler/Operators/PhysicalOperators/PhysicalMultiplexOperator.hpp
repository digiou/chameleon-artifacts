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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_PHYSICALMULTIPLEXOPERATOR_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_PHYSICALMULTIPLEXOPERATOR_HPP_
#include <Operators/AbstractOperators/Arity/ExchangeOperatorNode.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalOperator.hpp>
namespace x {
namespace QueryCompilation {
namespace PhysicalOperators {
/**
 * @brief The physical multiplex operator
 * This operator has multiple upstream operators and forwards it to exactly one down-stream operator.
 * Thus it has multiple child nodes and one parent nodes.
 * Example query plan:
 *
 * DataSource --- OperatorX ---
 *                             \
 *                              --- Multiplex --- OperatorZ --- DataSink
 *                              /
 * DataSource --- OperatorY ---
 *
 */
class PhysicalMultiplexOperator : public PhysicalOperator, public ExchangeOperatorNode {
  public:
    PhysicalMultiplexOperator(OperatorId id, const SchemaPtr& schema);
    static PhysicalOperatorPtr create(OperatorId id, const SchemaPtr& schema);
    static PhysicalOperatorPtr create(SchemaPtr schema);
    std::string toString() const override;
    OperatorNodePtr copy() override;
};

}// namespace PhysicalOperators
}// namespace QueryCompilation
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_PHYSICALMULTIPLEXOPERATOR_HPP_
