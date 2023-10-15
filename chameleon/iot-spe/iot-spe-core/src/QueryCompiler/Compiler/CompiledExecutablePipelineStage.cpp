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

#include <Compiler/DynamicObject.hpp>
#include <QueryCompiler/Compiler/CompiledExecutablePipelixtage.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>
namespace x {

// TODO this might change across OS
#if defined(__linux__) || defined(__APPLE__)
static constexpr auto MANGELED_ENTRY_POINT = "_ZN3x6createEv";
#else
#error "unsupported platform/OS"
#endif

using CreateFunctionPtr = Runtime::Execution::ExecutablePipelixtagePtr (*)();

CompiledExecutablePipelixtage::CompiledExecutablePipelixtage(std::shared_ptr<Compiler::DynamicObject> dynamicObject,
                                                                 PipelixtageArity arity,
                                                                 std::string sourceCode)
    : base(arity), dynamicObject(dynamicObject), currentExecutionStage(ExecutionStage::NotInitialized),
      sourceCode(std::move(sourceCode)) {
    auto createFunction = dynamicObject->getInvocableMember<CreateFunctionPtr>(MANGELED_ENTRY_POINT);
    this->executablePipelixtage = (*createFunction)();
}

Runtime::Execution::ExecutablePipelixtagePtr
CompiledExecutablePipelixtage::create(std::shared_ptr<Compiler::DynamicObject> dynamicObject,
                                        PipelixtageArity arity,
                                        const std::string& sourceCode) {
    return std::make_shared<CompiledExecutablePipelixtage>(dynamicObject, arity, sourceCode);
}

CompiledExecutablePipelixtage::~CompiledExecutablePipelixtage() {
    // First we have to destroy the pipeline stage only afterwards we can remove the associated code.
    this->executablePipelixtage.reset();
    this->dynamicObject.reset();
}

uint32_t CompiledExecutablePipelixtage::setup(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    const std::lock_guard<std::mutex> lock(executionStageLock);
    if (currentExecutionStage != ExecutionStage::NotInitialized) {
        x_FATAL_ERROR("CompiledExecutablePipelixtage: The pipeline stage, is already initialized."
                        "It is not allowed to call setup multiple times.");
        return -1;
    }
    currentExecutionStage = ExecutionStage::Initialized;
    return executablePipelixtage->setup(pipelineExecutionContext);
}

uint32_t CompiledExecutablePipelixtage::start(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    const std::lock_guard<std::mutex> lock(executionStageLock);
    if (currentExecutionStage != ExecutionStage::Initialized) {
        x_FATAL_ERROR("CompiledExecutablePipelixtage: The pipeline stage, is not initialized."
                        "It is not allowed to call start if setup was not called.");
        return -1;
    }
    x_DEBUG("CompiledExecutablePipelixtage: Start compiled executable pipeline stage");
    currentExecutionStage = ExecutionStage::Running;
    return executablePipelixtage->start(pipelineExecutionContext);
}

uint32_t CompiledExecutablePipelixtage::open(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                               Runtime::WorkerContext& workerContext) {
    const std::lock_guard<std::mutex> lock(executionStageLock);
    if (currentExecutionStage != ExecutionStage::Running) {
        x_FATAL_ERROR(
            "CompiledExecutablePipelixtage:open The pipeline stage, was not correctly initialized and started. You must first "
            "call setup and start.");
        return -1;
    }
    return executablePipelixtage->open(pipelineExecutionContext, workerContext);
}

ExecutionResult CompiledExecutablePipelixtage::execute(TupleBuffer& inputTupleBuffer,
                                                         Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                                         Runtime::WorkerContext& workerContext) {
    // we dont get the lock here as we dont was to serialize the execution.
    // currentExecutionStage is an atomic so its still save to read it
    if (currentExecutionStage != ExecutionStage::Running) {
        x_DEBUG("CompiledExecutablePipelixtage:execute The pipeline stage, was not correctly initialized and started. You "
                  "must first "
                  "call setup and start.");
        // TODO we have to assure that execute is never called after stop.
        // This is somehow not working currently.
        return ExecutionResult::Error;
    }

    return executablePipelixtage->execute(inputTupleBuffer, pipelineExecutionContext, workerContext);
}

std::string CompiledExecutablePipelixtage::getCodeAsString() { return sourceCode; }

uint32_t CompiledExecutablePipelixtage::close(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                                Runtime::WorkerContext& workerContext) {
    const std::lock_guard<std::mutex> lock(executionStageLock);
    if (currentExecutionStage != ExecutionStage::Running) {
        x_FATAL_ERROR(
            "CompiledExecutablePipelixtage:close The pipeline stage, was not correctly initialized and started. You must first "
            "call setup and start.");
        return -1;
    }
    return this->executablePipelixtage->close(pipelineExecutionContext, workerContext);
}
uint32_t CompiledExecutablePipelixtage::stop(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    if (currentExecutionStage != ExecutionStage::Running) {
        x_FATAL_ERROR(
            "CompiledExecutablePipelixtage:stop The pipeline stage, was not correctly initialized and started. You must first "
            "call setup and start.");
        return -1;
    }
    x_DEBUG("CompiledExecutablePipelixtage: Stop compiled executable pipeline stage");
    currentExecutionStage = ExecutionStage::Stopped;
    return executablePipelixtage->stop(pipelineExecutionContext);
}

}// namespace x