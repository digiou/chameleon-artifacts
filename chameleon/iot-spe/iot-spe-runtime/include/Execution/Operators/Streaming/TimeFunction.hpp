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
#ifndef x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_TIMEFUNCTION_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_TIMEFUNCTION_HPP_

#include <Nautilus/Interface/DataTypes/Integer/Int.hpp>
#include <Nautilus/Interface/DataTypes/Value.hpp>

namespace x::Nautilus {
class Record;
}

namespace x::Runtime::Execution {
class RecordBuffer;
class ExecutionContext;

}// namespace x::Runtime::Execution

namespace x::Runtime::Execution::Expressions {
class Expression;
using ExpressionPtr = std::shared_ptr<Expression>;
}// namespace x::Runtime::Execution::Expressions

namespace x::Runtime::Execution::Operators {
using namespace Nautilus;
/**
 * @brief A time function, infers the timestamp of an record.
 * For ingestion time, this is determined by the creation ts in the buffer.
 * For event time, this is infered by a field in the record.
 */
class TimeFunction {
  public:
    virtual void open(Execution::ExecutionContext& ctx, Execution::RecordBuffer& buffer) = 0;
    virtual Value<UInt64> getTs(Execution::ExecutionContext& ctx, Record& record) = 0;
    virtual ~TimeFunction() = default;
};

/**
 * @brief Time function for event time windows
 */
class EventTimeFunction final : public TimeFunction {
  public:
    explicit EventTimeFunction(Expressions::ExpressionPtr timestampExpression);
    void open(Execution::ExecutionContext& ctx, Execution::RecordBuffer& buffer) override;
    Value<UInt64> getTs(Execution::ExecutionContext& ctx, Record& record) override;

  private:
    Expressions::ExpressionPtr timestampExpression;
};

/**
 * @brief Time function for ingestion time windows
 */
class IngestionTimeFunction final : public TimeFunction {
  public:
    void open(Execution::ExecutionContext& ctx, Execution::RecordBuffer& buffer) override;
    Nautilus::Value<UInt64> getTs(Execution::ExecutionContext& ctx, Nautilus::Record& record) override;
};

}// namespace x::Runtime::Execution::Operators
#endif// x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_TIMEFUNCTION_HPP_
