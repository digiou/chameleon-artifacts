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
#include <Execution/Operators/ExecutionContext.hpp>
#include <Execution/Pipelix/NautilusExecutablePipelixtage.hpp>
#include <Execution/Pipelix/PhysicalOperatorPipeline.hpp>
#include <Execution/RecordBuffer.hpp>
#include <Nautilus/IR/Types/StampFactory.hpp>

namespace x::Runtime::Execution {

NautilusExecutablePipelixtage::NautilusExecutablePipelixtage(
    std::shared_ptr<PhysicalOperatorPipeline> physicalOperatorPipeline)
    : physicalOperatorPipeline(physicalOperatorPipeline) {}

uint32_t NautilusExecutablePipelixtage::setup(PipelineExecutionContext& pipelineExecutionContext) {
    auto pipelineExecutionContextRef = Value<MemRef>((int8_t*) &pipelineExecutionContext);
    auto workerContextRef = Value<MemRef>((int8_t*) nullptr);
    auto ctx = ExecutionContext(workerContextRef, pipelineExecutionContextRef);
    physicalOperatorPipeline->getRootOperator()->setup(ctx);
    return 0;
}

ExecutionResult NautilusExecutablePipelixtage::execute(TupleBuffer& inputTupleBuffer,
                                                         PipelineExecutionContext& pipelineExecutionContext,
                                                         WorkerContext& workerContext) {
    auto pipelineExecutionContextRef = Value<MemRef>((int8_t*) &pipelineExecutionContext);
    auto workerContextRef = Value<MemRef>((int8_t*) &workerContext);
    auto ctx = ExecutionContext(workerContextRef, pipelineExecutionContextRef);
    auto bufferRef = Value<MemRef>((int8_t*) std::addressof(inputTupleBuffer));
    auto recordBuffer = RecordBuffer(bufferRef);
    physicalOperatorPipeline->getRootOperator()->open(ctx, recordBuffer);
    physicalOperatorPipeline->getRootOperator()->close(ctx, recordBuffer);
    return ExecutionResult::Finished;
}

uint32_t NautilusExecutablePipelixtage::start(PipelineExecutionContext&) {
    // nop as we don't need this function in nautilus
    return 0;
}

uint32_t NautilusExecutablePipelixtage::open(Execution::PipelineExecutionContext&, WorkerContext&) {
    // nop as we don't need this function in nautilus
    return 0;
}
uint32_t NautilusExecutablePipelixtage::close(PipelineExecutionContext&, WorkerContext&) {
    // nop as we don't need this function in nautilus
    return 0;
}

uint32_t NautilusExecutablePipelixtage::stop(PipelineExecutionContext& pipelineExecutionContext) {
    auto pipelineExecutionContextRef = Value<MemRef>((int8_t*) &pipelineExecutionContext);
    auto workerContextRef = Value<MemRef>((int8_t*) nullptr);
    auto ctx = ExecutionContext(workerContextRef, pipelineExecutionContextRef);
    physicalOperatorPipeline->getRootOperator()->terminate(ctx);
    return 0;
}

std::string NautilusExecutablePipelixtage::getCodeAsString() { return "<no_code>"; }

}// namespace x::Runtime::Execution