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
#ifndef x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_CONSTANTVALUE_HPP_
#define x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_CONSTANTVALUE_HPP_
#include <memory>
namespace x::Nautilus {
class Any;
typedef std::shared_ptr<Any> AnyPtr;
}// namespace x::Nautilus
namespace x::Nautilus::Tracing {

/**
 * @brief Captures a constant value in the trace.
 */
class ConstantValue {
  public:
    ConstantValue(const AnyPtr& anyPtr);
    AnyPtr value;
    friend std::ostream& operator<<(std::ostream& os, const ConstantValue& tag);
};
}// namespace x::Nautilus::Tracing

#endif// x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_CONSTANTVALUE_HPP_
