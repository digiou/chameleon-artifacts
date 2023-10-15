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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/ExecutableQueryPlanStatus.hpp>
#include <Runtime/ExecutionResult.hpp>
#include <atomic>
#include <mutex>
namespace x {

namespace Compiler {
class DynamicObject;
}

using Runtime::TupleBuffer;
using Runtime::WorkerContext;
using Runtime::Execution::PipelineExecutionContext;
/**
 * @brief The CompiledExecutablePipelixtage maintains a reference to an compiled ExecutablePipelixtage.
 * To this end, it ensures that the compiled code is correctly destructed.
 */
class CompiledExecutablePipelixtage : public Runtime::Execution::ExecutablePipelixtage {
    using base = Runtime::Execution::ExecutablePipelixtage;

  public:
    /**
     * @brief This constructs a compiled pipeline
     * @param compiledCode pointer to compiled code
     * @param arity of the pipeline, e.g., binary or unary
     * @param sourceCode as string
     */
    explicit CompiledExecutablePipelixtage(std::shared_ptr<Compiler::DynamicObject> dynamicObject,
                                             PipelixtageArity arity,
                                             std::string sourceCode);
    static Runtime::Execution::ExecutablePipelixtagePtr
    create(std::shared_ptr<Compiler::DynamicObject> dynamicObject, PipelixtageArity arity, const std::string& sourceCode = "");
    ~CompiledExecutablePipelixtage();

    uint32_t setup(PipelineExecutionContext& pipelineExecutionContext) override;
    uint32_t start(PipelineExecutionContext& pipelineExecutionContext) override;
    uint32_t open(PipelineExecutionContext& pipelineExecutionContext, WorkerContext& workerContext) override;
    ExecutionResult execute(TupleBuffer& inputTupleBuffer,
                            PipelineExecutionContext& pipelineExecutionContext,
                            Runtime::WorkerContext& workerContext) override;

    std::string getCodeAsString() override;

    uint32_t close(PipelineExecutionContext& pipelineExecutionContext, WorkerContext& workerContext) override;
    uint32_t stop(PipelineExecutionContext& pipelineExecutionContext) override;

  private:
    enum class ExecutionStage : uint8_t { NotInitialized, Initialized, Running, Stopped };
    Runtime::Execution::ExecutablePipelixtagePtr executablePipelixtage;
    std::shared_ptr<Compiler::DynamicObject> dynamicObject;
    std::mutex executionStageLock;
    std::atomic<ExecutionStage> currentExecutionStage;
    std::string sourceCode;
};

}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_COMPILER_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
