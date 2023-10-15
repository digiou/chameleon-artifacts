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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_OPERATORPIPELINE_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_OPERATORPIPELINE_HPP_

#include <Nodes/Node.hpp>
#include <QueryCompiler/QueryCompilerForwardDeclaration.hpp>
#include <memory>
#include <vector>
namespace x {
namespace QueryCompilation {

/**
 * @brief Defix a single pipeline, which contains of a query plan of operators.
 * Each pipeline can have N successor and predecessor pipelix.
 */
class OperatorPipeline : public std::enable_shared_from_this<OperatorPipeline> {
  public:
    /**
     * @brief The type of a pipeline.
     * Source/Sink pipelix only have a single source and sink operator.
     * Operator pipelix consist of arbitrary operators, except sources and sinks.
     */
    enum class Type : uint8_t { SourcePipelineType, SinkPipelineType, OperatorPipelineType };

    /**
     * @brief Creates a new operator pipeline
     * @return OperatorPipelinePtr
     */
    static OperatorPipelinePtr create();

    /**
     * @brief Creates a new source pipeline.
     * @return OperatorPipelinePtr
     */
    static OperatorPipelinePtr createSourcePipeline();

    /**
     * @brief Creates a new sink pipeline.
     * @return OperatorPipelinePtr
     */
    static OperatorPipelinePtr createSinkPipeline();

    /**
     * @brief Adds a successor pipeline to the current one.
     * @param successor
     */
    void addSuccessor(const OperatorPipelinePtr& pipeline);

    /**
     * @brief Adds a predecessor pipeline to the current one.
     * @param predecessor
     */
    void addPredecessor(const OperatorPipelinePtr& pipeline);

    /**
     * @brief Removes a particular predecessor pipeline.
     * @param predecessor
     */
    void removePredecessor(const OperatorPipelinePtr& pipeline);

    /**
     * @brief Removes a particular successor pipeline.
     * @param successor
     */
    void removeSuccessor(const OperatorPipelinePtr& pipeline);

    /**
     * @brief Gets list of all predecessors
     * @return std::vector<OperatorPipelinePtr>
     */
    std::vector<OperatorPipelinePtr> getPredecessors() const;

    /**
     * @brief Gets list of all sucessors
     * @return std::vector<OperatorPipelinePtr>
     */
    std::vector<OperatorPipelinePtr> const& getSuccessors() const;

    /**
     * @brief Removes all predecessors
     */
    void clearPredecessors();

    /**
     * @brief Removes all successors
     */
    void clearSuccessors();

    /**
     * @brief Returns the query plan
     * @return QueryPlanPtr
     */
    QueryPlanPtr getQueryPlan();

    /**
     * @brief Returns the pipeline id
     * @return pipeline id.
     */
    uint64_t getPipelineId() const;

    /**
     * @brief Sets the type of an pipeline to Source, Sink, or Operator
     * @param pipelineType
     */
    void setType(Type pipelineType);

    /**
     * @brief Prepends a new operator to this pipeline.
     * @param newRootOperator
     */
    void prependOperator(OperatorNodePtr newRootOperator);

    /**
     * @brief Checks if this pipeline has an operator.
     * @return true if pipeline has an operator.
     */
    bool hasOperators() const;

    /**
     * @brief Indicates if this is a source pipeline.
     * @return true if source pipeline
     */
    bool isSourcePipeline() const;

    /**
     * @brief Indicates if this is a sink pipeline.
     * @return true if sink pipeline
     */
    bool isSinkPipeline() const;

    /**
     * @brief Indicates if this is a operator pipeline.
     * @return true if operator pipeline
     */
    bool isOperatorPipeline() const;
    const std::vector<uint64_t>& getOperatorIds() const;

    /**
     * @brief Creates a string representation of this OperatorPipeline
     * @return std::string
     */
    std::string toString() const;

  protected:
    OperatorPipeline(uint64_t pipelineId, Type pipelineType);

  private:
    uint64_t id;
    std::vector<std::shared_ptr<OperatorPipeline>> successorPipelix;
    std::vector<std::weak_ptr<OperatorPipeline>> predecessorPipelix;
    QueryPlanPtr queryPlan;
    std::vector<uint64_t> operatorIds;
    Type pipelineType;
};
}// namespace QueryCompilation

}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_OPERATORS_OPERATORPIPELINE_HPP_
