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

#include <Execution/Operators/Emit.hpp>
#include <Execution/Operators/ExecutionContext.hpp>
#include <Execution/Operators/OperatorState.hpp>
#include <Execution/RecordBuffer.hpp>
#include <Nautilus/Interface/Record.hpp>
#include <Util/StdInt.hpp>

namespace x::Runtime::Execution::Operators {

class EmitState : public OperatorState {
  public:
    explicit EmitState(const RecordBuffer& resultBuffer)
        : resultBuffer(resultBuffer), bufferReference(resultBuffer.getBuffer()) {}
    Value<UInt64> outputIndex = 0_u64;
    RecordBuffer resultBuffer;
    Value<MemRef> bufferReference;
};

void Emit::open(ExecutionContext& ctx, RecordBuffer&) const {
    // initialize state variable and create new buffer
    auto resultBufferRef = ctx.allocateBuffer();
    auto resultBuffer = RecordBuffer(resultBufferRef);
    ctx.setLocalOperatorState(this, std::make_unique<EmitState>(resultBuffer));
}

void Emit::execute(ExecutionContext& ctx, Record& record) const {
    auto emitState = (EmitState*) ctx.getLocalState(this);
    auto outputIndex = emitState->outputIndex;
    memoryProvider->write(outputIndex, emitState->bufferReference, record);
    emitState->outputIndex = outputIndex + 1_u64;
    // emit buffer if it reached the maximal capacity
    if (emitState->outputIndex >= maxRecordsPerBuffer) {
        auto resultBuffer = emitState->resultBuffer;
        resultBuffer.setNumRecords(emitState->outputIndex);
        resultBuffer.setWatermarkTs(ctx.getWatermarkTs());
        resultBuffer.setOriginId(ctx.getOriginId());
        resultBuffer.setSequenceNr(ctx.getSequenceNumber());
        ctx.emitBuffer(resultBuffer);
        auto resultBufferRef = ctx.allocateBuffer();
        emitState->resultBuffer = RecordBuffer(resultBufferRef);
        emitState->bufferReference = emitState->resultBuffer.getBuffer();
        emitState->outputIndex = 0_u64;
    }
}

void Emit::close(ExecutionContext& ctx, RecordBuffer&) const {
    // emit current buffer and set the number of records
    auto emitState = (EmitState*) ctx.getLocalState(this);
    auto resultBuffer = emitState->resultBuffer;
    resultBuffer.setNumRecords(emitState->outputIndex);
    resultBuffer.setWatermarkTs(ctx.getWatermarkTs());
    resultBuffer.setOriginId(ctx.getOriginId());
    resultBuffer.setSequenceNr(ctx.getSequenceNumber());
    ctx.emitBuffer(resultBuffer);
}

Emit::Emit(std::unique_ptr<MemoryProvider::MemoryProvider> memoryProvider)
    : maxRecordsPerBuffer(memoryProvider->getMemoryLayoutPtr()->getCapacity()), memoryProvider(std::move(memoryProvider)) {}

}// namespace x::Runtime::Execution::Operators