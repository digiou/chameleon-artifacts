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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_JOINING_STREAMING_PHYSICALxTEDLOOPJOINBUILDOPERATOR_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_JOINING_STREAMING_PHYSICALxTEDLOOPJOINBUILDOPERATOR_HPP_

#include <QueryCompiler/Operators/PhysicalOperators/Joining/Streaming/PhysicalxtedLoopJoinOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalUnaryOperator.hpp>

namespace x::QueryCompilation::PhysicalOperators {

/**
 * @brief This class represents the physical stream join build operator and gets translated to a StreamJoinBuild operator for the xted loop join
 */
class PhysicalxtedLoopJoinBuildOperator : public PhysicalxtedLoopJoinOperator,
                                            public PhysicalUnaryOperator,
                                            public AbstractEmitOperator {
  public:
    /**
     * @brief creates a PhysicalxtedLoopJoinBuildOperator with a provided operatorId
     * @param id
     * @param leftSchema
     * @param rightSchema
     * @param outputSchema
     * @param operatorHandler
     * @param buildSide
     * @param timeStampFieldName
     * @return PhysicalStreamJoinSinkOperator
     */
    static PhysicalOperatorPtr create(const OperatorId id,
                                      const SchemaPtr& inputSchema,
                                      const SchemaPtr& outputSchema,
                                      const Runtime::Execution::Operators::NLJOperatorHandlerPtr& operatorHandler,
                                      const JoinBuildSideType buildSide,
                                      const std::string& timeStampFieldName,
                                      const std::string& joinFieldName);

    /**
     * @brief creates a PhysicalxtedLoopJoinBuildOperator that retrieves a new operatorId by calling method
     * @param leftSchema
     * @param rightSchema
     * @param outputSchema
     * @param operatorHandler
     * @param buildSide
     * @param timeStampFieldName
     * @param joinFieldName
     * @return PhysicalStreamJoinBuildOperator
     */
    static PhysicalOperatorPtr create(const SchemaPtr& inputSchema,
                                      const SchemaPtr& outputSchema,
                                      const Runtime::Execution::Operators::NLJOperatorHandlerPtr& operatorHandler,
                                      const JoinBuildSideType buildSide,
                                      const std::string& timeStampFieldName,
                                      const std::string& joinFieldName);

    /**
     * @brief Constructor for PhysicalxtedLoopJoinBuildOperator
     * @param id
     * @param inputSchema
     * @param outputSchema
     * @param operatorHandler
     * @param buildSide
     * @param timeStampFieldName
     * @param joinFieldName
     */
    explicit PhysicalxtedLoopJoinBuildOperator(OperatorId id,
                                                 SchemaPtr inputSchema,
                                                 SchemaPtr outputSchema,
                                                 Runtime::Execution::Operators::NLJOperatorHandlerPtr operatorHandler,
                                                 JoinBuildSideType buildSide,
                                                 std::string timeStampFieldName,
                                                 std::string joinFieldName);

    /**
     * @brief Deconstructor
     */
    ~PhysicalxtedLoopJoinBuildOperator() noexcept override = default;

    /**
     * @brief Returns a string containing the name of this physical operator
     * @return String
     */
    [[nodiscard]] std::string toString() const override;

    /**
     * @brief Performs a deep copy of this physical operator
     * @return OperatorNodePtr
     */
    OperatorNodePtr copy() override;

    /**
     * @brief Getter for the build side, either left or right
     * @return JoinBuildSideType
     */
    JoinBuildSideType getBuildSide() const;

    /**
     * @brief Getter for the timestamp fieldname
     * @return String
     */
    const std::string& getTimeStampFieldName() const;

    /**
     * @brief Getter for the timestamp join field name
     * @return String
     */
    const std::string& getJoinFieldName() const;

  private:
    std::string timeStampFieldName;
    std::string joinFieldName;
    JoinBuildSideType buildSide;
};

}// namespace x::QueryCompilation::PhysicalOperators

#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_PHYSICALOPERATORS_JOINING_STREAMING_PHYSICALxTEDLOOPJOINBUILDOPERATOR_HPP_
