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

#ifndef x_CORE_INCLUDE_OPERATORS_ABSTRACTOPERATORS_ARITY_BINARYOPERATORNODE_HPP_
#define x_CORE_INCLUDE_OPERATORS_ABSTRACTOPERATORS_ARITY_BINARYOPERATORNODE_HPP_

#include <Operators/OperatorForwardDeclaration.hpp>
#include <Operators/OperatorNode.hpp>
namespace x {

/**
 * @brief A binary operator with more the none input operator, thus it has a left and a right input schema.
 */
class BinaryOperatorNode : public virtual OperatorNode {
  public:
    explicit BinaryOperatorNode(OperatorId id);

    /**
     * @brief detect if this operator is a binary operator, i.e., it has two children
     * @return true if n-ary else false;
     */
    bool isBinaryOperator() const override;

    /**
    * @brief detect if this operator is an binary operator, i.e., it has only one child
    * @return true if n-ary else false;
    */
    bool isUnaryOperator() const override;

    /**
   * @brief detect if this operator is an exchange operator, i.e., it sends it output to multiple parents
   * @return true if n-ary else false;
   */
    bool isExchangeOperator() const override;

    /**
   * @brief get the input schema of this operator from the left side
   * @return SchemaPtr
   */
    SchemaPtr getLeftInputSchema() const;

    /**
    * @brief set the input schema of this operator for the left side
     * @param inputSchema
    */
    void setLeftInputSchema(SchemaPtr inputSchema);

    /**
    * @brief get the input schema of this operator from the left side
    * @return SchemaPtr
    */
    SchemaPtr getRightInputSchema() const;

    /**
     * @brief set the input schema of this operator for the right side
     * @param inputSchema
    */
    void setRightInputSchema(SchemaPtr inputSchema);

    /**
    * @brief get the result schema of this operator
    * @return SchemaPtr
    */
    SchemaPtr getOutputSchema() const override;

    /**
     * @brief set the result schema of this operator
     * @param outputSchema
    */
    void setOutputSchema(SchemaPtr outputSchema) override;

    /**
     * @brief Set the input origin ids for the left input stream.
     * @param originIds
     */
    void setLeftInputOriginIds(std::vector<OriginId> originIds);

    /**
     * @brief Gets the input origin ids for the left input stream
     * @return std::vector<OriginId>
     */
    virtual std::vector<OriginId> getLeftInputOriginIds();

    /**
     * @brief Gets the input origin from both sides
     * @return std::vector<OriginId>
     */
    virtual std::vector<OriginId> getAllInputOriginIds();

    /**
     * @brief Set the input origin ids for the right input stream.
     * @param originIds
     */
    void setRightInputOriginIds(std::vector<OriginId> originIds);

    /**
     * @brief Gets the input origin ids for the right input stream
     * @return std::vector<OriginId>
     */
    virtual std::vector<OriginId> getRightInputOriginIds();

    /**
     * @brief Gets the output origin ids for the result stream
     * @return std::vector<OriginId> originids
     */
    const std::vector<OriginId> getOutputOriginIds() const override;

  protected:
    SchemaPtr leftInputSchema;
    SchemaPtr rightInputSchema;
    SchemaPtr outputSchema;
    std::vector<SchemaPtr> distinctSchemas;
    std::vector<OriginId> leftInputOriginIds;
    std::vector<OriginId> rightInputOriginIds;
};

}// namespace x

#endif// x_CORE_INCLUDE_OPERATORS_ABSTRACTOPERATORS_ARITY_BINARYOPERATORNODE_HPP_
