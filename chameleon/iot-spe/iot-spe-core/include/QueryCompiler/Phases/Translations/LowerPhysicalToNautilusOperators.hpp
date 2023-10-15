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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERPHYSICALTONAUTILUSOPERATORS_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERPHYSICALTONAUTILUSOPERATORS_HPP_

#include <Execution/Aggregation/AggregationFunction.hpp>
#include <Execution/Aggregation/AggregationValue.hpp>
#include <Execution/Expressions/Expression.hpp>
#include <Execution/Operators/Operator.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/StreamHashJoinOperatorHandler.hpp>
#include <Execution/Operators/Streaming/TimeFunction.hpp>
#include <Execution/Pipelix/PhysicalOperatorPipeline.hpp>
#include <Nodes/Expressions/ExpressionNode.hpp>
#include <QueryCompiler/Operators/OperatorPipeline.hpp>
#include <QueryCompiler/Operators/PipelineQueryPlan.hpp>
#include <QueryCompiler/QueryCompilerForwardDeclaration.hpp>
#include <Windowing/WindowAggregations/WindowAggregationDescriptor.hpp>
#include <cstddef>
#include <memory>
#include <vector>

namespace x::QueryCompilation {
class ExpressionProvider;

/**
 * @brief This phase lowers a pipeline plan of physical operators into a pipeline plan of nautilus operators.
 * The lowering of individual operators is defined by the nautilus operator provider to improve extendability.
 */
class LowerPhysicalToNautilusOperators {
  public:
    /**
     * @brief Constructor to create a LowerPhysicalToGeneratableOperatorPhase
     */
    explicit LowerPhysicalToNautilusOperators(const QueryCompilation::QueryCompilerOptionsPtr& options);

    /**
     * @brief Create a LowerPhysicalToGeneratableOperatorPhase
     */
    static std::shared_ptr<LowerPhysicalToNautilusOperators> create(const QueryCompilation::QueryCompilerOptionsPtr& options);

    /**
     * @brief Applies the phase on a pipelined query plan.
     * @param pipelined query plan
     * @return PipelineQueryPlanPtr
     */
    PipelineQueryPlanPtr apply(PipelineQueryPlanPtr pipelinedQueryPlan, size_t bufferSize);

    /**
     * @brief Applies the phase on a pipelined and lower physical operator to generatable once.
     * @param pipeline
     * @return OperatorPipelinePtr
     */
    OperatorPipelinePtr apply(OperatorPipelinePtr pipeline, size_t bufferSize);

    /**
     * @brief Deconstructor for this class
     */
    ~LowerPhysicalToNautilusOperators();

  private:
    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lower(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
          std::shared_ptr<Runtime::Execution::Operators::Operator> parentOperator,
          const PhysicalOperators::PhysicalOperatorPtr& operatorPtr,
          size_t bufferSize,
          std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lowerScan(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
              const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
              size_t bufferSize);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerEmit(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
              const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
              size_t bufferSize);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerFilter(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                const PhysicalOperators::PhysicalOperatorPtr& physicalOperator);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerLimit(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
               const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
               std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerMap(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
             const PhysicalOperators::PhysicalOperatorPtr& physicalOperator);

    std::unique_ptr<Runtime::Execution::Operators::TimeFunction>
    lowerTimeFunction(const Windowing::TimeBasedWindowTypePtr& timeBasedWindowType);

    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lowerNonKeyedSliceMergingOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                      const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                      std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lowerKeyedSliceMergingOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                   const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                   std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lowerKeyedSlidingWindowSinkOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                        const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                        std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerGlobalThreadLocalPreAggregationOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                                 const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                                 std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerKeyedThreadLocalPreAggregationOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                                const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                                std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerWatermarkAssignmentOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                     const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                     std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::Operator>
    lowerNonKeyedSlidingWindowSinkOperator(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                                           const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                                           std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);

    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerThresholdWindow(Runtime::Execution::PhysicalOperatorPipeline& pipeline,
                         const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                         uint64_t handlerIndex);

    std::vector<std::shared_ptr<Runtime::Execution::Aggregation::AggregationFunction>>
    lowerAggregations(const std::vector<Windowing::WindowAggregationPtr>& functions);

    /**
     * Create a unique pointer of an aggregation value of the given aggregation function then return it
     * @param aggregationType the type of this aggregation
     * @param inputType the data type of the input tuples for this aggregation
     * @return unique pointer of an aggregation value
     */
    std::unique_ptr<Runtime::Execution::Aggregation::AggregationValue>
    getAggregationValueForThresholdWindow(Windowing::WindowAggregationDescriptor::Type aggregationType, DataTypePtr inputType);

#ifdef TFDEF
    /**
     * @brief Creates an executable operator for an inference operation
     * @param physicalOperator
     * @param operatorHandlers
     * @return A shared pointer to an ExecutableOperator object that represents the infer model
     */
    std::shared_ptr<Runtime::Execution::Operators::ExecutableOperator>
    lowerInferModelOperator(const PhysicalOperators::PhysicalOperatorPtr& physicalOperator,
                            std::vector<Runtime::Execution::OperatorHandlerPtr>& operatorHandlers);
#endif

  private:
    const QueryCompilation::QueryCompilerOptionsPtr options;
    std::unique_ptr<ExpressionProvider> expressionProvider;
};
}// namespace x::QueryCompilation
#endif// x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERPHYSICALTONAUTILUSOPERATORS_HPP_
