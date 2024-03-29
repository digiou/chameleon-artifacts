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
#ifndef x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
#include "Nautilus/IR/IRGraph.hpp"
#include "Util/Timer.hpp"
#include <Execution/Pipelix/NautilusExecutablePipelixtage.hpp>
#include <Nautilus/Backends/Executable.hpp>
#include <Nautilus/Util/CompilationOptions.hpp>
#include <future>
namespace x {
class DumpHelper;
}
namespace x::Nautilus::Backends {
class Executable;
}
namespace x::Nautilus::IR {
class IRGraph;
}
namespace x::Runtime::Execution {
class PhysicalOperatorPipeline;

/**
 * @brief A compiled executable pipeline stage uses nautilus to compile a pipeline to a code snippet.
 */
class CompiledExecutablePipelixtage : public NautilusExecutablePipelixtage {
  public:
    CompiledExecutablePipelixtage(const std::shared_ptr<PhysicalOperatorPipeline>& physicalOperatorPipeline,
                                    const std::string& compilationBackend,
                                    const Nautilus::CompilationOptions& options);
    uint32_t setup(PipelineExecutionContext& pipelineExecutionContext) override;
    ExecutionResult execute(TupleBuffer& inputTupleBuffer,
                            PipelineExecutionContext& pipelineExecutionContext,
                            WorkerContext& workerContext) override;
    std::shared_ptr<x::Nautilus::IR::IRGraph> createIR(DumpHelper& dumpHelper, Timer<>& timer);

  private:
    std::unique_ptr<Nautilus::Backends::Executable> compilePipeline();
    std::string compilationBackend;
    const Nautilus::CompilationOptions options;
    std::shared_future<std::unique_ptr<Nautilus::Backends::Executable>> executablePipeline;
    Nautilus::Backends::Executable::Invocable<void, void*, void*, void*> pipelineFunction;
};

}// namespace x::Runtime::Execution

#endif// x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILEDEXECUTABLEPIPELIxTAGE_HPP_
