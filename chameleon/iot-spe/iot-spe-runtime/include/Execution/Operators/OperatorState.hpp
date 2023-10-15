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

#ifndef x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_OPERATORSTATE_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_OPERATORSTATE_HPP_
namespace x::Runtime::Execution::Operators {

/**
 * @brief Base class of all operator state.
 * Actual operators should inherit from this.
 */
class OperatorState {
  public:
    virtual ~OperatorState() = default;
};
}// namespace x::Runtime::Execution::Operators
#endif// x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_OPERATORSTATE_HPP_
