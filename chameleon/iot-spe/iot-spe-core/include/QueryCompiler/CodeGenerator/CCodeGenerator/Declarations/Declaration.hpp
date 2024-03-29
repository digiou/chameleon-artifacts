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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_DECLARATIONS_DECLARATION_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_DECLARATIONS_DECLARATION_HPP_

#include <QueryCompiler/CodeGenerator/CodeGeneratorForwardRef.hpp>
#include <memory>
#include <string>
namespace x {

namespace QueryCompilation {

class Declaration {
  public:
    Declaration() = default;
    Declaration(const Declaration&) = default;
    virtual ~Declaration() = default;
    [[nodiscard]] virtual GeneratableDataTypePtr getType() const = 0;
    [[nodiscard]] virtual std::string getIdentifierName() const = 0;
    [[nodiscard]] virtual Code getTypeDefinitionCode() const = 0;
    [[nodiscard]] virtual Code getCode() const = 0;
    [[nodiscard]] virtual DeclarationPtr copy() const = 0;
};

class StructDeclaration;
const DataTypePtr createUserDefinedType(const StructDeclaration& decl);
}// namespace QueryCompilation
}// namespace x
#endif// x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_DECLARATIONS_DECLARATION_HPP_
