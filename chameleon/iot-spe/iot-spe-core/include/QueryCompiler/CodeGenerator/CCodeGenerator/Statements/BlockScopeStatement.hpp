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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_BLOCKSCOPESTATEMENT_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_BLOCKSCOPESTATEMENT_HPP_

#include <QueryCompiler/CodeGenerator/CCodeGenerator/Statements/CompoundStatement.hpp>
#include <QueryCompiler/CodeGenerator/CodeGeneratorForwardRef.hpp>
namespace x {
namespace QueryCompilation {

/**
 * @brief A block scope statement
 * {
 * ....
 * }
 */
class BlockScopeStatement : public CompoundStatement {
  public:
    static BlockScopeStatementPtr create();

    [[nodiscard]] CodeExpressionPtr getCode() const override;
};
}// namespace QueryCompilation
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_STATEMENTS_BLOCKSCOPESTATEMENT_HPP_
