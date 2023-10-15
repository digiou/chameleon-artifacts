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
#include <Operators/OperatorNode.hpp>
#include <QueryCompiler/Operators/ExecutableOperator.hpp>
#include <Util/Core.hpp>
#include <utility>
namespace x::QueryCompilation {

ExecutableOperator::ExecutableOperator(OperatorId id,
                                       Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage,
                                       std::vector<Runtime::Execution::OperatorHandlerPtr> operatorHandlers)
    : OperatorNode(id), UnaryOperatorNode(id), executablePipelixtage(std::move(executablePipelixtage)),
      operatorHandlers(std::move(operatorHandlers)) {}

OperatorNodePtr ExecutableOperator::create(Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage,
                                           std::vector<Runtime::Execution::OperatorHandlerPtr> operatorHandlers) {
    return std::make_shared<ExecutableOperator>(
        ExecutableOperator(Util::getNextOperatorId(), std::move(executablePipelixtage), std::move(operatorHandlers)));
}

Runtime::Execution::ExecutablePipelixtagePtr ExecutableOperator::getExecutablePipelixtage() {
    return executablePipelixtage;
}

std::vector<Runtime::Execution::OperatorHandlerPtr> ExecutableOperator::getOperatorHandlers() { return operatorHandlers; }

std::string ExecutableOperator::toString() const { return "ExecutableOperator"; }

OperatorNodePtr ExecutableOperator::copy() { return create(executablePipelixtage, operatorHandlers); }

}// namespace x::QueryCompilation