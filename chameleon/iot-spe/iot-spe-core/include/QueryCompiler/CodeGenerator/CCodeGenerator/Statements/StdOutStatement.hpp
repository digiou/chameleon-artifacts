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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_STDOUTSTATEMENT_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_STDOUTSTATEMENT_HPP_

#include <QueryCompiler/CodeGenerator/CCodeGenerator/Statements/Statement.hpp>
namespace x::QueryCompilation {
/**
 * @brief this statements allows us to generate code printing to std::out
 */
class StdOutStatement : public Statement {
  public:
    template<typename T>
    inline StdOutStatement(T&& m) noexcept : message(std::forward<T>(m)) {}

    ~StdOutStatement() noexcept override = default;

    [[nodiscard]] StatementType getStamentType() const override;

    [[nodiscard]] CodeExpressionPtr getCode() const override;

    [[nodiscard]] StatementPtr createCopy() const override;

  private:
    std::string message;
};

using StdOut = StdOutStatement;
}// namespace x::QueryCompilation

#endif// x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_STDOUTSTATEMENT_HPP_
