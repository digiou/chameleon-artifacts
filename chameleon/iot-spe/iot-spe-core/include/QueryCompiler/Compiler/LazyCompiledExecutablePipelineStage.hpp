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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_LAZYCOMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_LAZYCOMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/ExecutableQueryPlanStatus.hpp>
#include <Runtime/ExecutionResult.hpp>
#include <atomic>
#include <future>
#include <mutex>

namespace x {
class LazyCompiledExecutablePipelixtage : public Runtime::Execution::ExecutablePipelixtage {
  public:
    LazyCompiledExecutablePipelixtage(std::shared_future<Runtime::Execution::ExecutablePipelixtagePtr> futurePipelixtage);
    static Runtime::Execution::ExecutablePipelixtagePtr
    create(std::shared_future<Runtime::Execution::ExecutablePipelixtagePtr> futurePipeline);
    uint32_t setup(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) override;
    uint32_t start(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) override;
    uint32_t open(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                  Runtime::WorkerContext& workerContext) override;
    ExecutionResult execute(Runtime::TupleBuffer& inputTupleBuffer,
                            Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                            Runtime::WorkerContext& workerContext) override;
    uint32_t close(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext,
                   Runtime::WorkerContext& workerContext) override;
    uint32_t stop(Runtime::Execution::PipelineExecutionContext& pipelineExecutionContext) override;
    ~LazyCompiledExecutablePipelixtage() override;
    std::string getCodeAsString() override;

  private:
    std::shared_future<Runtime::Execution::ExecutablePipelixtagePtr> futurePipelixtage;
};
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_LAZYCOMPILEDEXECUTABLEPIPELIxTAGE_HPP_
