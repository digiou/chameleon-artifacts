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
#ifndef x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALPREAGGREGATIONOPERATORHANDLER_HPP_
#define x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALPREAGGREGATIONOPERATORHANDLER_HPP_

#include <Runtime/Execution/OperatorHandler.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedThreadLocalSliceStore.hpp>
namespace x::Experimental {
class HashMapFactory;
using HashMapFactoryPtr = std::shared_ptr<HashMapFactory>;
class LockFreeMultiOriginWatermarkProcessor;
}// namespace x::Experimental

namespace x::Windowing {
class LogicalWindowDefinition;
using LogicalWindowDefinitionPtr = std::shared_ptr<LogicalWindowDefinition>;
}// namespace x::Windowing

namespace x::Windowing::Experimental {
class NonKeyedSliceStaging;

/**
 * @brief The GlobalThreadLocalPreAggregationOperatorHandler provides an operator handler to perform slice-based pre-aggregation
 * of global non-keyed tumbling and sliding windows.
 * This operator handler, maintains a slice store for each worker thread and provides them for the aggregation.
 * For each processed tuple buffer triggerThreadLocalState is called, which checks if the thread-local slice store should be triggered.
 */
class NonKeyedThreadLocalPreAggregationOperatorHandler
    : public Runtime::Execution::OperatorHandler,
      public detail::virtual_enable_shared_from_this<NonKeyedThreadLocalPreAggregationOperatorHandler, false> {
    using inherited0 = detail::virtual_enable_shared_from_this<NonKeyedThreadLocalPreAggregationOperatorHandler, false>;
    using inherited1 = Runtime::Reconfigurable;

  public:
    /**
     * @brief Creates the operator handler with a specific window definition, a set of origins, and access to the slice staging object.
     * @param windowDefinition logical window definition
     * @param origins the set of origins, which can produce data for the window operator
     * @param weakSliceStagingPtr access to the slice staging.
     */
    NonKeyedThreadLocalPreAggregationOperatorHandler(const Windowing::LogicalWindowDefinitionPtr& windowDefinition,
                                                     const std::vector<OriginId> origins,
                                                     std::weak_ptr<NonKeyedSliceStaging> weakSliceStagingPtr);

    /**
     * @brief Initializes the thread local state for the window operator
     * @param ctx PipelineExecutionContext
     * @param entrySize Size of the aggregated values in memory
     */
    void setup(Runtime::Execution::PipelineExecutionContext& ctx, uint64_t entrySize);

    void start(Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext,
               Runtime::StateManagerPtr stateManager,
               uint32_t localStateVariableId) override;

    /**
     * @brief Stops the operator handler and triggers all in flight slices.
     * @param pipelineExecutionContext pipeline execution context
     */
    void stop(Runtime::QueryTerminationType queryTerminationType,
              Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext) override;

    /**
     * @brief This method triggers the thread local state and appends all slices,
     * which end before the current global watermark to the slice staging area.
     * @param wctx WorkerContext
     * @param ctx PipelineExecutionContext
     * @param workerId
     * @param originId
     * @param sequenceNumber
     * @param watermarkTs
     */
    void triggerThreadLocalState(Runtime::WorkerContext& wctx,
                                 Runtime::Execution::PipelineExecutionContext& ctx,
                                 uint64_t workerId,
                                 OriginId originId,
                                 uint64_t sequenceNumber,
                                 uint64_t watermarkTs);

    /**
     * @brief Returns the thread local slice store by a specific worker thread id
     * @param workerId
     * @return GlobalThreadLocalSliceStore
     */
    NonKeyedThreadLocalSliceStore& getThreadLocalSliceStore(uint64_t workerId);

    /**
     * @brief Returns the window definition.
     * @return Windowing::LogicalWindowDefinitionPtr
     */
    Windowing::LogicalWindowDefinitionPtr getWindowDefinition();

    ~NonKeyedThreadLocalPreAggregationOperatorHandler();

    void postReconfigurationCallback(Runtime::ReconfigurationMessage& message) override;

  private:
    uint64_t windowSize;
    uint64_t windowSlide;
    std::weak_ptr<NonKeyedSliceStaging> weakSliceStaging;
    std::vector<std::unique_ptr<NonKeyedThreadLocalSliceStore>> threadLocalSliceStores;
    std::shared_ptr<::x::Experimental::LockFreeMultiOriginWatermarkProcessor> watermarkProcessor;
    Windowing::LogicalWindowDefinitionPtr windowDefinition;
    x::Experimental::HashMapFactoryPtr factory;
};
}// namespace x::Windowing::Experimental

#endif// x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALPREAGGREGATIONOPERATORHANDLER_HPP_
