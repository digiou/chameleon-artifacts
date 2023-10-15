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

#include <QueryCompiler/CodeGenerator/CodeExpression.hpp>
#include <QueryCompiler/GeneratableTypes/PointerDataType.hpp>
#include <utility>

namespace x::QueryCompilation {
PointerDataType::PointerDataType(GeneratableDataTypePtr baseType) : baseType(std::move(baseType)) {}

CodeExpressionPtr PointerDataType::getCode() const { return std::make_shared<CodeExpression>(baseType->getCode()->code_ + "*"); }

CodeExpressionPtr PointerDataType::getTypeDefinitionCode() const { return baseType->getTypeDefinitionCode(); }

CodeExpressionPtr PointerDataType::getDeclarationCode(std::string identifier) const {
    return std::make_shared<CodeExpression>(baseType->getCode()->code_ + "* " + identifier);
}
}// namespace x::QueryCompilation