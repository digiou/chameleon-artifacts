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

#include <Runtime/BufferManager.hpp>
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Util/Experimental/HashMap.hpp>
#include <Util/NonBlockingMonotonicSeqQueue.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedGlobalSliceStoreAppendOperatorHandler.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSlice.hpp>
#include <Windowing/Experimental/WindowProcessingTasks.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/WindowMeasures/TimeMeasure.hpp>
#include <Windowing/WindowTypes/TimeBasedWindowType.hpp>
namespace x::Windowing::Experimental {

NonKeyedGlobalSliceStoreAppendOperatorHandler::NonKeyedGlobalSliceStoreAppendOperatorHandler(
    const Windowing::LogicalWindowDefinitionPtr& windowDefinition,
    std::weak_ptr<GlobalSliceStore<NonKeyedSlice>> globalSliceStore)
    : globalSliceStore(globalSliceStore), windowDefinition(windowDefinition) {
    auto timeBasedWindowType = Windowing::WindowType::asTimeBasedWindowType(windowDefinition->getWindowType());
    windowSize = timeBasedWindowType->getSize().getTime();
    windowSlide = timeBasedWindowType->getSlide().getTime();
}

void NonKeyedGlobalSliceStoreAppendOperatorHandler::start(Runtime::Execution::PipelineExecutionContextPtr,
                                                          Runtime::StateManagerPtr,
                                                          uint32_t) {
    x_DEBUG("start NonKeyedGlobalSliceStoreAppendOperatorHandler");
}

void NonKeyedGlobalSliceStoreAppendOperatorHandler::triggerSliceMerging(Runtime::WorkerContext& wctx,
                                                                        Runtime::Execution::PipelineExecutionContext& ctx,
                                                                        uint64_t sequenceNumber,
                                                                        NonKeyedSlicePtr slice) {
    auto global = globalSliceStore.lock();
    if (!global) {
        x_FATAL_ERROR("GlobalSliceStore is invalid, this should only happen after a hard stop. Drop all in flight data.");
        return;
    }
    // add pre-aggregated slice to slice store
    auto windows = global->addSliceAndTriggerWindows(sequenceNumber, std::move(slice), windowSize, windowSlide);
    // check if we can trigger window computation
    for (auto& window : windows) {
        auto buffer = wctx.allocateTupleBuffer();
        auto task = buffer.getBuffer<WindowTriggerTask>();
        // we trigger the completion of all windows that end between startSlice and <= endSlice.
        x_DEBUG("Deploy window trigger task for slice  ( {}-{})", window.startTs, window.endTs);
        task->windowStart = window.startTs;
        task->windowEnd = window.endTs;
        task->sequenceNumber = window.sequenceNumber;
        buffer.setNumberOfTuples(1);
        ctx.dispatchBuffer(buffer);
    }
}

void NonKeyedGlobalSliceStoreAppendOperatorHandler::stop(Runtime::QueryTerminationType queryTerminationType,
                                                         Runtime::Execution::PipelineExecutionContextPtr ctx) {
    x_DEBUG("stop NonKeyedGlobalSliceStoreAppendOperatorHandler : {}", queryTerminationType);
    if (queryTerminationType == Runtime::QueryTerminationType::Graceful) {
        auto global = globalSliceStore.lock();
        x_ASSERT(global, "Global slice store is invalid. This should not happen in a graceful stop.");
        auto windows = global->triggerAllInflightWindows(windowSize, windowSlide);
        for (auto& window : windows) {
            auto buffer = ctx->getBufferManager()->getBufferBlocking();
            auto task = buffer.getBuffer<WindowTriggerTask>();
            // we trigger the completion of all windows that end between startSlice and <= endSlice.
            x_DEBUG("Deploy window trigger task for slice  ( {}-{})", window.startTs, window.endTs);
            task->windowStart = window.startTs;
            task->windowEnd = window.endTs;
            task->sequenceNumber = window.sequenceNumber;
            buffer.setNumberOfTuples(1);
            ctx->dispatchBuffer(buffer);
        }
    }
}

NonKeyedGlobalSliceStoreAppendOperatorHandler::~NonKeyedGlobalSliceStoreAppendOperatorHandler() {
    x_DEBUG("Destruct NonKeyedGlobalSliceStoreAppendOperatorHandler");
}

Windowing::LogicalWindowDefinitionPtr NonKeyedGlobalSliceStoreAppendOperatorHandler::getWindowDefinition() {
    return windowDefinition;
}

}// namespace x::Windowing::Experimental