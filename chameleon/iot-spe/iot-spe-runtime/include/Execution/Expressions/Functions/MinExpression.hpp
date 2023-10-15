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

#ifndef x_RUNTIME_INCLUDE_EXECUTION_EXPRESSIONS_FUNCTIONS_MINEXPRESSION_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_EXPRESSIONS_FUNCTIONS_MINEXPRESSION_HPP_

#include <Execution/Expressions/Expression.hpp>
#include <Nautilus/Interface/DataTypes/Value.hpp>

namespace x::Runtime::Execution::Expressions {

/**
 * @brief This expression returns the minimum of the leftSubExpression and the rightSubExpression.
 */
class MinExpression : public Expression {
  public:
    MinExpression(const ExpressionPtr& leftSubExpression, const ExpressionPtr& rightSubExpression);
    Value<> execute(Record& record) const override;

  private:
    const ExpressionPtr leftSubExpression;
    const ExpressionPtr rightSubExpression;
};

}// namespace x::Runtime::Execution::Expressions

#endif// x_RUNTIME_INCLUDE_EXECUTION_EXPRESSIONS_FUNCTIONS_MINEXPRESSION_HPP_