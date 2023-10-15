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

#ifndef x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_RELATIONAL_AGGREGATION_BATCHAGGREGATIONSCAN_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_RELATIONAL_AGGREGATION_BATCHAGGREGATIONSCAN_HPP_
#include <Execution/Aggregation/AggregationFunction.hpp>
#include <Execution/Expressions/Expression.hpp>
#include <Execution/Operators/ExecutableOperator.hpp>

namespace x::Runtime::Execution::Operators {

/**
 * @brief Batch Aggregation scan operator, it accesses multiple thread local states and performs the final aggregation.
 */
class BatchAggregationScan : public Operator {
  public:
    /**
     * @brief Creates a batch aggregation operator with a expression.
     */
    BatchAggregationScan(uint64_t operatorHandlerIndex,
                         const std::vector<std::shared_ptr<Execution::Aggregation::AggregationFunction>>& aggregationFunctions);
    void open(ExecutionContext& ctx, RecordBuffer& recordBuffer) const override;

  private:
    const uint64_t operatorHandlerIndex;
    const std::vector<std::shared_ptr<Aggregation::AggregationFunction>> aggregationFunctions;
};
}// namespace x::Runtime::Execution::Operators
#endif// x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_RELATIONAL_AGGREGATION_BATCHAGGREGATIONSCAN_HPP_