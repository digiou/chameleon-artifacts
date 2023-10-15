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

#include <Common/ValueTypes/BasicValue.hpp>
#include <QueryCompiler/CodeGenerator/CodeExpression.hpp>
#include <QueryCompiler/GeneratableTypes/GeneratableBasicValueType.hpp>
#include <utility>

namespace x::QueryCompilation {
GeneratableBasicValueType::GeneratableBasicValueType(BasicValuePtr basicValue) : value(std::move(basicValue)) {}

CodeExpressionPtr GeneratableBasicValueType::getCodeExpression() const noexcept {
    return std::make_shared<CodeExpression>(value->value);
}
}// namespace x::QueryCompilation