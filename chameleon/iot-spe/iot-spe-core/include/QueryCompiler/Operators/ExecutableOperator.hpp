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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_EXECUTABLEOPERATOR_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_EXECUTABLEOPERATOR_HPP_
#include <Operators/AbstractOperators/Arity/UnaryOperatorNode.hpp>
#include <QueryCompiler/QueryCompilerForwardDeclaration.hpp>
namespace x {
namespace QueryCompilation {

/**
 * @brief A executable operator, represents an executable version of one or more operators in a query plan.
 * It is currently used to represent compiled executable pipeline stages in a query plan.
 * Based on this query plan we then create the executable query plan.
 */
class ExecutableOperator : public UnaryOperatorNode {
  public:
    /**
     * @brief Creates a new executable operator, which captures a pipeline stage and a set of operator handlers.
     * @param executablePipelixtage the executable pipeline stage
     * @param operatorHandlers a list of operator handlers
     * @return OperatorNodePtr
     */
    static OperatorNodePtr create(Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage,
                                  std::vector<Runtime::Execution::OperatorHandlerPtr> operatorHandlers);

    /**
     * @brief Gets the executable pipeline stage.
     * @return ExecutablePipelixtagePtr
     */
    Runtime::Execution::ExecutablePipelixtagePtr getExecutablePipelixtage();

    /**
     * @brief Gets the operator handlers, which capture specific operator state.
     * @return std::vector<Runtime::Execution::OperatorHandlerPtr>
     */
    std::vector<Runtime::Execution::OperatorHandlerPtr> getOperatorHandlers();
    std::string toString() const override;
    OperatorNodePtr copy() override;

  private:
    ExecutableOperator(OperatorId id,
                       Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage,
                       std::vector<Runtime::Execution::OperatorHandlerPtr> operatorHandlers);
    Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage;
    std::vector<Runtime::Execution::OperatorHandlerPtr> operatorHandlers;
};

}// namespace QueryCompilation
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_EXECUTABLEOPERATOR_HPP_
