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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_GENERATABLEOPERATORS_GENERATABLEBUFFERSCAN_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_GENERATABLEOPERATORS_GENERATABLEBUFFERSCAN_HPP_

#include <QueryCompiler/Operators/GeneratableOperators/GeneratableOperator.hpp>

namespace x {
namespace QueryCompilation {
namespace GeneratableOperators {

/**
 * @brief Generates a for loop, which iterates over the input buffer.
 */
class GeneratableBufferScan : public GeneratableOperator {
  public:
    /**
     * @brief Creates a new generatable buffer scan operator.
     * @param inputSchema the schema, which describes the input of an operator.
     * @return GeneratableOperatorPtr
     */
    static GeneratableOperatorPtr create(SchemaPtr inputSchema);

    /**
    * @brief Creates a new generatable buffer scan operator.
    * @param id operator id
    * @param inputSchema the schema, which describes the input of an operator.
    * @return GeneratableOperatorPtr
    */
    static GeneratableOperatorPtr create(OperatorId id, SchemaPtr inputSchema);

    /**
     * @brief Code generation function for the open call of an operator.
     * The open function is called once per operator to initialize local state by a single thread.
     * @param codegen reference to the code generator.
     * @param context reference to the current pipeline context.
     */
    void generateOpen(CodeGeneratorPtr codegen, PipelineContextPtr context) override;

    /**
    * @brief Code generation function for the execute call of an operator.
    * The execute function is called for each tuple buffer consumed by this operator.
    * @param codegen reference to the code generator.
    * @param context reference to the current pipeline context.
    */
    void generateExecute(CodeGeneratorPtr codegen, PipelineContextPtr context) override;

    [[nodiscard]] std::string toString() const override;

    OperatorNodePtr copy() override;

    ~GeneratableBufferScan() noexcept override = default;

  private:
    GeneratableBufferScan(OperatorId id, const SchemaPtr& inputSchema);
};
}// namespace GeneratableOperators
}// namespace QueryCompilation
}// namespace x
#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_GENERATABLEOPERATORS_GENERATABLEBUFFERSCAN_HPP_
