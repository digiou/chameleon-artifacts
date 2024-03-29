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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_LOGICALOPERATORFACTORY_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_LOGICALOPERATORFACTORY_HPP_

#include <Nodes/Expressions/ConstantValueExpressionNode.hpp>
#include <Operators/LogicalOperators/LogicalOperatorForwardRefs.hpp>
#include <Operators/OperatorForwardDeclaration.hpp>

#include <Util/Core.hpp>
#include <Windowing/WindowingForwardRefs.hpp>

namespace x {
class ExpressionItem;

class LogicalOperatorFactory {
  public:
    template<typename Operator, typename... Arguments>
    static std::shared_ptr<Operator> create(Arguments&&... args) {
        return std::shared_ptr<Operator>(std::forward<Arguments>(args)...);
    }

    /**
     * @brief Create a new logical filter operator.
     * @param predicate: the filter predicate is represented as an expression node, which has to return true.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createFilterOperator(ExpressionNodePtr const& predicate,
                                                            OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new source rename operator.
     * @param new source name
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createRenameSourceOperator(std::string const& newSourceName,
                                                                  OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new logical limit operator.
     * @param limit number of tuples to output
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createLimitOperator(const uint64_t limit, OperatorId id = Util::getNextOperatorId());

    /**
    * @brief Create a new logical projection operator.
    * @param expression list
    * @param id: the id of the operator if not defined then next free operator id is used.
    * @return LogicalOperatorNodePtr
    */
    static LogicalUnaryOperatorNodePtr createProjectionOperator(const std::vector<ExpressionNodePtr>& expressions,
                                                                OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new sink operator with a specific sink descriptor.
     * @param sinkDescriptor the SinkDescriptor.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return LogicalOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createSinkOperator(SinkDescriptorPtr const& sinkDescriptor,
                                                          OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new map operator with a field assignment expression as a map expression.
     * @param mapExpression the FieldAssignmentExpressionNode.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createMapOperator(FieldAssignmentExpressionNodePtr const& mapExpression,
                                                         OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new infer model operator.
     * @param model: The path to the model of the operator.
     * @param inputFields: The intput fields of the model.
     * @param outputFields: The output fields of the model.
     * @param id: The id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createInferModelOperator(std::string model,
                                                                std::vector<ExpressionItemPtr> inputFields,
                                                                std::vector<ExpressionItemPtr> outputFields,
                                                                OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new source operator with source descriptor.
     * @param sourceDescriptor the SourceDescriptorPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createSourceOperator(SourceDescriptorPtr const& sourceDescriptor,
                                                            OperatorId id = Util::getNextOperatorId(),
                                                            OriginId originId = INVALID_ORIGIN_ID);

    /**
    * @brief Create a specialized watermark assigner operator.
    * @param watermarkStrategy strategy to be used to assign the watermark
    * @param id: the id of the operator if not defined then next free operator id is used.
    * @return LogicalOperatorNodePtr
    */
    static LogicalUnaryOperatorNodePtr
    createWatermarkAssignerOperator(Windowing::WatermarkStrategyDescriptorPtr const& watermarkStrategyDescriptor,
                                    OperatorId id = Util::getNextOperatorId());
    /**
     * @brief Create a new window operator with window definition.
     * @param windowDefinition the LogicalWindowDefinitionPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr createWindowOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition,
                                                            OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a specialized central window operator with window definition.
     * @param windowDefinition the LogicalWindowDefinitionPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr
    createCentralWindowSpecializedOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition,
                                           OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a specialized slice creation window operator with window definition.
     * @param windowDefinition the LogicalWindowDefinitionPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr
    createSliceCreationSpecializedOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition,
                                           OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a specialized slice merging window operator with window definition.
     * @param windowDefinition the LogicalWindowDefinitionPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr
    createSliceMergingSpecializedOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition,
                                          OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a specialized window computation window operator with window definition.
     * @param windowDefinition the LogicalWindowDefinitionPtr.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return UnaryOperatorNodePtr
     */
    static LogicalUnaryOperatorNodePtr
    createWindowComputationSpecializedOperator(Windowing::LogicalWindowDefinitionPtr const& windowDefinition,
                                               OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a specialized union operator.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return BinaryOperatorNode
     */
    static LogicalBinaryOperatorNodePtr createUnionOperator(OperatorId id = Util::getNextOperatorId());

    /**
    * @brief Create a specialized join operator.
    * @param id: the id of the operator if not defined then next free operator id is used.
    * @return BinaryOperatorNode
    */
    static LogicalBinaryOperatorNodePtr createJoinOperator(const Join::LogicalJoinDefinitionPtr& joinDefinition,
                                                           OperatorId id = Util::getNextOperatorId());

    // todo put in experimental namespace
    /**
    * @brief Create a specialized batch join operator.
    * @param id: the id of the operator if not defined then next free operator id is used.
    * @return BinaryOperatorNode
    */
    static LogicalBinaryOperatorNodePtr
    createBatchJoinOperator(const Join::Experimental::LogicalBatchJoinDefinitionPtr& batchJoinDefinition,
                            OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a broadcast operator.
     * @param id: the id of the operator if not defined then next free operator id is used.
     * @return BroadcastLogicalOperatorNodePtr
     */
    static BroadcastLogicalOperatorNodePtr createBroadcastOperator(OperatorId id = Util::getNextOperatorId());

    /**
     * CEP Operators
     */

    /**
    * @brief Create a new logical iteration operator.
    * @param minIterations  the minimal expected number of iterations
    * @param maxIterations  the maximal expected number of iterations, if 0 unlimited
    * @param id: the id of the operator if not defined then next free operator id is used.
    * @return LogicalOperatorNodePtr
    */
    static LogicalUnaryOperatorNodePtr
    createCEPIterationOperator(uint64_t minIterations, uint64_t maxIterations, OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new MapJavaUDFLogicalOperatorNode.
     * @param javaUdfDescriptor The descriptor of the Java UDF represented by this logical operator node.
     * @param id The operator ID.
     * @return A logical operator node which encapsulates the Java UDF.
     */
    static LogicalUnaryOperatorNodePtr createMapUDFLogicalOperator(const Catalogs::UDF::UDFDescriptorPtr udfDescriptor,
                                                                   OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new FlatMapJavaUDFLogicalOperatorNode.
     * @param javaUdfDescriptor The descriptor of the Java UDF represented by this logical operator node.
     * @param id The operator ID.
     * @return A logical operator node which encapsulates the Java UDF.
     */
    static LogicalUnaryOperatorNodePtr createFlatMapUDFLogicalOperator(const Catalogs::UDF::UDFDescriptorPtr udfDescriptor,
                                                                       OperatorId id = Util::getNextOperatorId());

    /**
     * @brief Create a new OpenCL logical operator
     * @param javaUdfDescriptor : the java UDF descriptor
     * @param id : the id of the operator
     * @return a logical operator of type OpenCL logical operator
     */
    static LogicalUnaryOperatorNodePtr createOpenCLLogicalOperator(const Catalogs::UDF::JavaUdfDescriptorPtr javaUdfDescriptor,
                                                                   OperatorId id = Util::getNextOperatorId());
};

}// namespace x

#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_LOGICALOPERATORFACTORY_HPP_
