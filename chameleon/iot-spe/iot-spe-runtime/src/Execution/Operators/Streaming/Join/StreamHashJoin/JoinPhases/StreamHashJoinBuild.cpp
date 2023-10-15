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
#include <API/AttributeField.hpp>
#include <Common/DataTypes/DataType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Execution/Expressions/ReadFieldExpression.hpp>
#include <Execution/Operators/ExecutionContext.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/DataStructure/LocalHashTable.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/JoinPhases/StreamHashJoinBuild.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/StreamHashJoinOperatorHandler.hpp>
#include <Execution/Operators/Streaming/TimeFunction.hpp>
#include <Execution/RecordBuffer.hpp>
#include <Nautilus/Interface/FunctionCall.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <atomic>

namespace x::Runtime::Execution::Operators {
class LocalGlobalJoinState : public Operators::OperatorState {
  public:
    LocalGlobalJoinState(Value<MemRef>& operatorHandler, Value<MemRef>& hashTableReference, Value<MemRef>& windowReference)
        : joinOperatorHandler(operatorHandler), hashTableReference(hashTableReference), windowReference(windowReference),
          windowStart((uint64_t) 0), windowEnd((uint64_t) 0){};
    Value<MemRef> joinOperatorHandler;
    Value<MemRef> hashTableReference;
    Value<MemRef> windowReference;
    Value<UInt64> windowStart;
    Value<UInt64> windowEnd;
};

void* getStreamHashJoinWindowProxy(void* ptrOpHandler, uint64_t timeStamp) {
    x_DEBUG("getStreamHashJoinWindowProxy with ts={}", timeStamp);
    StreamHashJoinOperatorHandler* opHandler = static_cast<StreamHashJoinOperatorHandler*>(ptrOpHandler);
    auto currentWindow = opHandler->getWindowByTimestampOrCreateIt(timeStamp);
    x_ASSERT2_FMT(currentWindow != nullptr, "invalid window");
    StreamHashJoinWindow* hashWindow = static_cast<StreamHashJoinWindow*>(currentWindow.get());
    return static_cast<void*>(hashWindow);
}

uint64_t getWindowStartProxy(void* ptrHashWindow) {
    x_ASSERT2_FMT(ptrHashWindow != nullptr, "hash window handler context should not be null");
    StreamHashJoinWindow* hashWindow = static_cast<StreamHashJoinWindow*>(ptrHashWindow);
    return hashWindow->getWindowStart();
}

uint64_t getWindowEndProxy(void* ptrHashWindow) {
    x_ASSERT2_FMT(ptrHashWindow != nullptr, "hash window handler context should not be null");
    StreamHashJoinWindow* hashWindow = static_cast<StreamHashJoinWindow*>(ptrHashWindow);
    return hashWindow->getWindowEnd();
}

void* getLocalHashTableProxy(void* ptrHashWindow, size_t workerIdx, uint64_t joinBuildSideInt) {
    x_ASSERT2_FMT(ptrHashWindow != nullptr, "hash window handler context should not be null");
    auto* hashWindow = static_cast<StreamHashJoinWindow*>(ptrHashWindow);
    auto joinBuildSide = magic_enum::enum_cast<QueryCompilation::JoinBuildSideType>(joinBuildSideInt).value();
    x_DEBUG("Insert into HT for window={} is left={} workerIdx={}",
              hashWindow->getWindowIdentifier(),
              magic_enum::enum_name(joinBuildSide),
              workerIdx);
    auto ptr = hashWindow->getHashTable(joinBuildSide, workerIdx);
    auto localHashTablePointer = static_cast<void*>(ptr);
    return localHashTablePointer;
}

void* insertFunctionProxy(void* ptrLocalHashTable, uint64_t key) {
    x_ASSERT2_FMT(ptrLocalHashTable != nullptr, "ptrLocalHashTable should not be null");
    LocalHashTable* localHashTable = static_cast<LocalHashTable*>(ptrLocalHashTable);
    return localHashTable->insert(key);
}

/**
* @brief Updates the windowState of all windows and emits buffers, if the windows can be emitted
*/
void checkWindowsTriggerProxyForJoinBuild(void* ptrOpHandler,
                                          void* ptrPipelineCtx,
                                          void* ptrWorkerCtx,
                                          uint64_t watermarkTs,
                                          uint64_t sequenceNumber,
                                          OriginId originId) {
    x_ASSERT2_FMT(ptrOpHandler != nullptr, "opHandler context should not be null!");
    x_ASSERT2_FMT(ptrPipelineCtx != nullptr, "pipeline context should not be null");
    x_ASSERT2_FMT(ptrWorkerCtx != nullptr, "worker context should not be null");

    auto* opHandler = static_cast<StreamHashJoinOperatorHandler*>(ptrOpHandler);
    auto pipelineCtx = static_cast<PipelineExecutionContext*>(ptrPipelineCtx);
    auto workerCtx = static_cast<WorkerContext*>(ptrWorkerCtx);

    //update last seen watermark by this worker
    opHandler->updateWatermarkForWorker(watermarkTs, workerCtx->getId());
    auto minWatermark = opHandler->getMinWatermarkForWorker();

    auto windowIdentifiersToBeTriggered = opHandler->checkWindowsTrigger(minWatermark, sequenceNumber, originId);

    //TODO I am not sure do we have to make sure that only one thread do this? issue #3909
    if (windowIdentifiersToBeTriggered.size() > 0) {
        opHandler->triggerWindows(windowIdentifiersToBeTriggered, workerCtx, pipelineCtx);
    }
}
void* getDefaultMemRef() { return nullptr; }

void StreamHashJoinBuild::open(ExecutionContext& ctx, RecordBuffer&) const {
    auto operatorHandlerMemRef = ctx.getGlobalOperatorHandler(handlerIndex);
    Value<MemRef> dummyRef1 = Nautilus::FunctionCall("getDefaultMemRef", getDefaultMemRef);
    Value<MemRef> dummyRef2 = Nautilus::FunctionCall("getDefaultMemRef", getDefaultMemRef);
    auto joinState = std::make_unique<LocalGlobalJoinState>(operatorHandlerMemRef, dummyRef1, dummyRef2);
    ctx.setLocalOperatorState(this, std::move(joinState));
}

void StreamHashJoinBuild::execute(ExecutionContext& ctx, Record& record) const {
    auto joinState = static_cast<LocalGlobalJoinState*>(ctx.getLocalState(this));
    auto operatorHandlerMemRef = joinState->joinOperatorHandler;
    Value<UInt64> tsValue = timeFunction->getTs(ctx, record);

    //check if we can reuse window
    if (!(joinState->windowStart <= tsValue && tsValue < joinState->windowEnd)) {
        //we need a new window
        joinState->windowReference = Nautilus::FunctionCall("getStreamHashJoinWindowProxy",
                                                            getStreamHashJoinWindowProxy,
                                                            operatorHandlerMemRef,
                                                            Value<UInt64>(tsValue));

        joinState->hashTableReference = Nautilus::FunctionCall("getLocalHashTableProxy",
                                                               getLocalHashTableProxy,
                                                               joinState->windowReference,
                                                               ctx.getWorkerId(),
                                                               Value<UInt64>((uint64_t) magic_enum::enum_integer(joinBuildSide)));

        joinState->windowStart = Nautilus::FunctionCall("getWindowStartProxy", getWindowStartProxy, joinState->windowReference);

        joinState->windowEnd = Nautilus::FunctionCall("getWindowEndProxy", getWindowEndProxy, joinState->windowReference);
        x_DEBUG("reinit join state with start={} end={} for ts={} for isLeftSide={}",
                  joinState->windowStart->toString(),
                  joinState->windowEnd->toString(),
                  tsValue->toString(),
                  magic_enum::enum_name(joinBuildSide));
    }

    //get position in the HT where to write to auto physicalDataTypeFactory = DefaultPhysicalTypeFactory();
    auto entryMemRef = Nautilus::FunctionCall("insertFunctionProxy",
                                              insertFunctionProxy,
                                              joinState->hashTableReference,
                                              record.read(joinFieldName).as<UInt64>());
    //write data
    auto physicalDataTypeFactory = DefaultPhysicalTypeFactory();
    for (auto& field : inputSchema->fields) {
        auto const fieldName = field->getName();
        auto const fieldType = physicalDataTypeFactory.getPhysicalType(field->getDataType());
        x_TRACE("write key={} value={}", field->getName(), record.read(fieldName)->toString());
        entryMemRef.store(record.read(fieldName));
        entryMemRef = entryMemRef + fieldType->size();
    }
}

void StreamHashJoinBuild::close(ExecutionContext& ctx, RecordBuffer&) const {
    auto operatorHandlerMemRef = ctx.getGlobalOperatorHandler(handlerIndex);
    // Update the watermark and trigger windows
    Nautilus::FunctionCall("checkWindowsTriggerProxy",
                           checkWindowsTriggerProxyForJoinBuild,
                           operatorHandlerMemRef,
                           ctx.getPipelineContext(),
                           ctx.getWorkerContext(),
                           ctx.getWatermarkTs(),
                           ctx.getSequenceNumber(),
                           ctx.getOriginId());
}

void setupOperatorHandler(void* ptrOpHandler, void* ptrPipelineCtx) {
    x_ASSERT(ptrOpHandler != nullptr, "op handler context should not be null");
    x_ASSERT(ptrPipelineCtx != nullptr, "pipeline context should not be null");

    auto opHandler = static_cast<StreamHashJoinOperatorHandler*>(ptrOpHandler);
    auto pipelineCtx = static_cast<PipelineExecutionContext*>(ptrPipelineCtx);

    opHandler->setup(pipelineCtx->getNumberOfWorkerThreads());
}

void StreamHashJoinBuild::setup(ExecutionContext& ctx) const {
    auto operatorHandlerMemRef = ctx.getGlobalOperatorHandler(handlerIndex);
    Nautilus::FunctionCall("setupOperatorHandler", setupOperatorHandler, operatorHandlerMemRef, ctx.getPipelineContext());
}

StreamHashJoinBuild::StreamHashJoinBuild(uint64_t handlerIndex,
                                         const QueryCompilation::JoinBuildSideType joinBuildSide,
                                         const std::string& joinFieldName,
                                         const std::string& timeStampField,
                                         SchemaPtr inputSchema,
                                         TimeFunctionPtr timeFunction)
    : handlerIndex(handlerIndex), joinBuildSide(joinBuildSide), joinFieldName(joinFieldName), timeStampField(timeStampField),
      inputSchema(inputSchema), timeFunction(std::move(timeFunction)) {}

}// namespace x::Runtime::Execution::Operators