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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_WINDOWING_PHYSICALSLICEPREAGGREGATIONOPERATOR_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_WINDOWING_PHYSICALSLICEPREAGGREGATIONOPERATOR_HPP_

#include <QueryCompiler/Operators/PhysicalOperators/AbstractEmitOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/Windowing/PhysicalWindowOperator.hpp>

namespace x::QueryCompilation::PhysicalOperators {

/**
 * @brief Physical operator for slice pre-aggregation.
 * This operator receives records and pre-aggregates values in a slice store.
 */
class PhysicalSlicePreAggregationOperator : public PhysicalWindowOperator, public AbstractEmitOperator {
  public:
    PhysicalSlicePreAggregationOperator(OperatorId id,
                                        SchemaPtr inputSchema,
                                        SchemaPtr outputSchema,
                                        Windowing::WindowOperatorHandlerPtr handler);
    static PhysicalOperatorPtr create(OperatorId id,
                                      const SchemaPtr& inputSchema,
                                      const SchemaPtr& outputSchema,
                                      const Windowing::WindowOperatorHandlerPtr& handler);
    static PhysicalOperatorPtr create(SchemaPtr inputSchema, SchemaPtr outputSchema, Windowing::WindowOperatorHandlerPtr handler);
    std::string toString() const override;
    OperatorNodePtr copy() override;
};
}// namespace x::QueryCompilation::PhysicalOperators

#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_WINDOWING_PHYSICALSLICEPREAGGREGATIONOPERATOR_HPP_