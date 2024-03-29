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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_RECORDHANDLER_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_RECORDHANDLER_HPP_
#include <QueryCompiler/CodeGenerator/CodeGeneratorForwardRef.hpp>
#include <map>
#include <string>
namespace x {
namespace QueryCompilation {

/**
 * @brief The record handler allows a generatable
 * operator to access and modify attributes of the current source record.
 * Furthermore, it guarantees that modifications to record attributes are correctly tracked.
 */
class RecordHandler {
  public:
    static RecordHandlerPtr create();

    /**
     * @brief Registers a new attribute to the record handler.
     * @param name attribute name.
     * @param variableAccessStatement reference to the statement that generates this attribute.
     */
    void registerAttribute(const std::string& name, ExpressionStatementPtr variableAccessStatement);

    /**
     * @brief Checks a specific attribute was already registered.
     * @param name attribute name.
     * @return true if attribute is registered
     */
    bool hasAttribute(const std::string& name);

    /**
     * @brief Returns the VariableReference to the particular attribute name.
     * @param name attribute name
     * @return VariableDeclarationPtr
     */
    ExpressionStatementPtr getAttribute(const std::string& name);

  private:
    std::map<std::string, ExpressionStatementPtr> statementMap;
};
}// namespace QueryCompilation
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_RECORDHANDLER_HPP_
