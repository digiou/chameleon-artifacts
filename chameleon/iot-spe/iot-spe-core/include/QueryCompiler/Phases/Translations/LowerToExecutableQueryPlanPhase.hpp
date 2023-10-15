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
#ifndef x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERTOEXECUTABLEQUERYPLANPHASE_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERTOEXECUTABLEQUERYPLANPHASE_HPP_

#include <QueryCompiler/QueryCompilerForwardDeclaration.hpp>
#include <Runtime/Execution/ExecutableQueryPlan.hpp>

#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <vector>

namespace x {

class PhysicalSource;
using PhysicalSourcePtr = std::shared_ptr<PhysicalSource>;

namespace QueryCompilation {

class LowerToExecutableQueryPlanPhase {
  public:
    LowerToExecutableQueryPlanPhase(DataSinkProviderPtr sinkProvider, DataSourceProviderPtr sourceProvider);
    static LowerToExecutableQueryPlanPhasePtr create(const DataSinkProviderPtr& sinkProvider,
                                                     const DataSourceProviderPtr& sourceProvider);

    Runtime::Execution::ExecutableQueryPlanPtr apply(const PipelineQueryPlanPtr& pipelineQueryPlan,
                                                     const Runtime::NodeEnginePtr& nodeEngine);

  private:
    DataSinkProviderPtr sinkProvider;
    DataSourceProviderPtr sourceProvider;
    void processSource(const OperatorPipelinePtr& pipeline,
                       std::vector<DataSourcePtr>& sources,
                       std::vector<DataSinkPtr>& sinks,
                       std::vector<Runtime::Execution::ExecutablePipelinePtr>& executablePipelix,
                       const Runtime::NodeEnginePtr& nodeEngine,
                       const PipelineQueryPlanPtr& pipelineQueryPlan,
                       std::map<uint64_t, Runtime::Execution::SuccessorExecutablePipeline>& pipelineToExecutableMap);

    Runtime::Execution::SuccessorExecutablePipeline
    processSuccessor(const OperatorPipelinePtr& pipeline,
                     std::vector<DataSourcePtr>& sources,
                     std::vector<DataSinkPtr>& sinks,
                     std::vector<Runtime::Execution::ExecutablePipelinePtr>& executablePipelix,
                     const Runtime::NodeEnginePtr& nodeEngine,
                     const PipelineQueryPlanPtr& pipelineQueryPlan,
                     std::map<uint64_t, Runtime::Execution::SuccessorExecutablePipeline>& pipelineToExecutableMap);

    Runtime::Execution::SuccessorExecutablePipeline
    processSink(const OperatorPipelinePtr& pipeline,
                std::vector<DataSourcePtr>& sources,
                std::vector<DataSinkPtr>& sinks,
                std::vector<Runtime::Execution::ExecutablePipelinePtr>& executablePipelix,
                Runtime::NodeEnginePtr nodeEngine,
                const PipelineQueryPlanPtr& pipelineQueryPlan);

    Runtime::Execution::SuccessorExecutablePipeline
    processOperatorPipeline(const OperatorPipelinePtr& pipeline,
                            std::vector<DataSourcePtr>& sources,
                            std::vector<DataSinkPtr>& sinks,
                            std::vector<Runtime::Execution::ExecutablePipelinePtr>& executablePipelix,
                            const Runtime::NodeEnginePtr& nodeEngine,
                            const PipelineQueryPlanPtr& pipelineQueryPlan,
                            std::map<uint64_t, Runtime::Execution::SuccessorExecutablePipeline>& pipelineToExecutableMap);

    /**
     * @brief Create Actual Source descriptor from default source descriptor and Physical source properties
     * @param defaultSourceDescriptor: the default source descriptor
     * @param physicalSource : the physical source
     * @return Shared pointer for actual source descriptor
     */
    SourceDescriptorPtr createSourceDescriptor(SchemaPtr schema, PhysicalSourcePtr physicalSource);
};
}// namespace QueryCompilation
}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_PHASES_TRANSLATIONS_LOWERTOEXECUTABLEQUERYPLANPHASE_HPP_
