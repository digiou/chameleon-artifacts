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
#ifndef x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILATIONPIPELINEPROVIDER_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILATIONPIPELINEPROVIDER_HPP_
#include <Execution/Pipelix/ExecutablePipelineProvider.hpp>
#include <Nautilus/Util/CompilationOptions.hpp>
namespace x::Runtime::Execution {

/**
 * @brief Creates an executable pipeline stage that can be executed using compilation.
 */
class CompilationPipelineProvider : public ExecutablePipelineProvider {
  public:
    std::unique_ptr<ExecutablePipelixtage> create(std::shared_ptr<PhysicalOperatorPipeline> physicalOperatorPipeline,
                                                    const Nautilus::CompilationOptions& options) override;
};
}// namespace x::Runtime::Execution
#endif// x_RUNTIME_INCLUDE_EXECUTION_PIPELIx_COMPILATIONPIPELINEPROVIDER_HPP_
