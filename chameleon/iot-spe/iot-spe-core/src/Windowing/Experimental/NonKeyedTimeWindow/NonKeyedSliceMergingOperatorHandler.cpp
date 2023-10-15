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
#include <Runtime/WorkerContext.hpp>
#include <Util/NonBlockingMonotonicSeqQueue.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSlice.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSliceMergingOperatorHandler.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSliceStaging.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedThreadLocalSliceStore.hpp>
#include <Windowing/Experimental/WindowProcessingTasks.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/WindowMeasures/TimeMeasure.hpp>
#include <Windowing/WindowTypes/WindowType.hpp>
namespace x::Windowing::Experimental {

NonKeyedSliceMergingOperatorHandler::NonKeyedSliceMergingOperatorHandler(
    const Windowing::LogicalWindowDefinitionPtr& windowDefinition)
    : sliceStaging(std::make_shared<NonKeyedSliceStaging>()), windowDefinition(windowDefinition) {}

void NonKeyedSliceMergingOperatorHandler::setup(Runtime::Execution::PipelineExecutionContext&, uint64_t entrySize) {
    this->entrySize = entrySize;
}

void NonKeyedSliceMergingOperatorHandler::start(Runtime::Execution::PipelineExecutionContextPtr,
                                                Runtime::StateManagerPtr,
                                                uint32_t) {
    x_DEBUG("start NonKeyedSliceMergingOperatorHandler");
}

void NonKeyedSliceMergingOperatorHandler::stop(Runtime::QueryTerminationType queryTerminationType,
                                               Runtime::Execution::PipelineExecutionContextPtr) {
    x_DEBUG("stop NonKeyedSliceMergingOperatorHandler: {}", queryTerminationType);
}

NonKeyedSlicePtr NonKeyedSliceMergingOperatorHandler::createGlobalSlice(SliceMergeTask* sliceMergeTask) {
    return std::make_unique<NonKeyedSlice>(entrySize, sliceMergeTask->startSlice, sliceMergeTask->endSlice);
}

NonKeyedSliceMergingOperatorHandler::~NonKeyedSliceMergingOperatorHandler() {
    x_DEBUG("Destruct NonKeyedSliceMergingOperatorHandler");
}

Windowing::LogicalWindowDefinitionPtr NonKeyedSliceMergingOperatorHandler::getWindowDefinition() { return windowDefinition; }

std::weak_ptr<NonKeyedSliceStaging> NonKeyedSliceMergingOperatorHandler::getSliceStagingPtr() { return sliceStaging; }

}// namespace x::Windowing::Experimental