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

#include <QueryCompiler/CodeGenerator/CCodeGenerator/Statements/BinaryOperatorStatement.hpp>
#include <utility>

namespace x::QueryCompilation {
StatementPtr ExpressionStatement::createCopy() const { return this->copy(); }

BinaryOperatorStatement ExpressionStatement::operator[](const ExpressionStatement& ref) {
    return BinaryOperatorStatement(*this, BinaryOperatorType::ARRAY_REFERENCE_OP, ref);
}

BinaryOperatorStatement ExpressionStatement::accessPtr(const ExpressionStatement& ref) {
    return BinaryOperatorStatement(*this, BinaryOperatorType::MEMBER_SELECT_POINTER_OP, ref);
}

BinaryOperatorStatement ExpressionStatement::accessPtr(const ExpressionStatementPtr& ref) const {
    return BinaryOperatorStatement(this->copy(), BinaryOperatorType::MEMBER_SELECT_POINTER_OP, ref);
}

BinaryOperatorStatement ExpressionStatement::accessRef(const ExpressionStatement& ref) {
    return BinaryOperatorStatement(*this, BinaryOperatorType::MEMBER_SELECT_REFERENCE_OP, ref);
}

BinaryOperatorStatement ExpressionStatement::accessRef(ExpressionStatementPtr ref) const {
    return BinaryOperatorStatement(this->copy(), BinaryOperatorType::MEMBER_SELECT_REFERENCE_OP, std::move(ref));
}

BinaryOperatorStatement ExpressionStatement::assign(const ExpressionStatement& ref) {
    return BinaryOperatorStatement(*this, BinaryOperatorType::ASSIGNMENT_OP, ref);
}

BinaryOperatorStatement ExpressionStatement::assign(ExpressionStatementPtr ref) const {
    return BinaryOperatorStatement(this->copy(), BinaryOperatorType::ASSIGNMENT_OP, std::move(ref));
}
}// namespace x::QueryCompilation
