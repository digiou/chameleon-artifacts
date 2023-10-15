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

#include <QueryCompiler/Compiler/LazyCompiledExecutablePipelixtage.hpp>
#include <Util/Logger/Logger.hpp>
namespace x {
x::LazyCompiledExecutablePipelixtage::LazyCompiledExecutablePipelixtage(
    std::shared_future<Runtime::Execution::ExecutablePipelixtagePtr> futurePipelixtage)
    : futurePipelixtage(futurePipelixtage) {}

uint32_t
x::LazyCompiledExecutablePipelixtage::setup(x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    return futurePipelixtage.get()->setup(pipelineExecutionContext);
}
uint32_t
x::LazyCompiledExecutablePipelixtage::start(x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    return futurePipelixtage.get()->start(pipelineExecutionContext);
}
uint32_t
x::LazyCompiledExecutablePipelixtage::open(x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                               x::Runtime::WorkerContext& workerContext) {
    return futurePipelixtage.get()->open(pipelineExecutionContext, workerContext);
}
x::ExecutionResult
x::LazyCompiledExecutablePipelixtage::execute(x::Runtime::TupleBuffer& inputTupleBuffer,
                                                  x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                                  x::Runtime::WorkerContext& workerContext) {
    return futurePipelixtage.get()->execute(inputTupleBuffer, pipelineExecutionContext, workerContext);
}
uint32_t
x::LazyCompiledExecutablePipelixtage::close(x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                                                x::Runtime::WorkerContext& workerContext) {
    return futurePipelixtage.get()->close(pipelineExecutionContext, workerContext);
}
uint32_t
x::LazyCompiledExecutablePipelixtage::stop(x::Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) {
    return futurePipelixtage.get()->stop(pipelineExecutionContext);
}
std::string x::LazyCompiledExecutablePipelixtage::getCodeAsString() { return futurePipelixtage.get()->getCodeAsString(); }
Runtime::Execution::ExecutablePipelixtagePtr
LazyCompiledExecutablePipelixtage::create(std::shared_future<Runtime::Execution::ExecutablePipelixtagePtr> futurePipeline) {
    return std::make_shared<LazyCompiledExecutablePipelixtage>(futurePipeline);
}
LazyCompiledExecutablePipelixtage::~LazyCompiledExecutablePipelixtage() {
    x_DEBUG("~LazyCompiledExecutablePipelixtage()");
}

}// namespace x