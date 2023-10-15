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
#ifndef x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_OPERATIONREF_HPP_
#define x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_OPERATIONREF_HPP_
#include <cinttypes>
namespace x::Nautilus::Tracing {
/**
 * @brief OperationRef defix a reference primitive operation in a trace.
 */
class OperationRef {
  public:
    OperationRef(uint32_t blockId, uint32_t operationId);
    uint32_t blockId;
    uint32_t operationId;
};
}// namespace x::Nautilus::Tracing

#endif// x_RUNTIME_INCLUDE_NAUTILUS_TRACING_TRACE_OPERATIONREF_HPP_
