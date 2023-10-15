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
#include <Execution/Pipelix/CompilationPipelineProvider.hpp>
#include <Execution/Pipelix/NautilusExecutablePipelixtage.hpp>
#include <Nodes/Util/Iterators/DepthFirstNodeIterator.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/Exceptions/QueryCompilationException.hpp>
#include <QueryCompiler/Operators/ExecutableOperator.hpp>
#include <QueryCompiler/Operators/NautilusPipelineOperator.hpp>
#include <QueryCompiler/Operators/OperatorPipeline.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/Joining/PhysicalBatchJoinBuildOperator.hpp>
#include <QueryCompiler/Operators/PipelineQueryPlan.hpp>
#include <QueryCompiler/Phases/NautilusCompilationPase.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <utility>
namespace x::QueryCompilation {

NautilusCompilationPhase::NautilusCompilationPhase(const QueryCompilation::QueryCompilerOptionsPtr& compilerOptions)
    : compilerOptions(compilerOptions) {}

std::shared_ptr<NautilusCompilationPhase>
NautilusCompilationPhase::create(const QueryCompilation::QueryCompilerOptionsPtr& compilerOptions) {
    return std::make_shared<NautilusCompilationPhase>(compilerOptions);
}

PipelineQueryPlanPtr NautilusCompilationPhase::apply(PipelineQueryPlanPtr queryPlan) {
    x_DEBUG("Generate code for query plan {} - {}", queryPlan->getQueryId(), queryPlan->getQuerySubPlanId());
    for (const auto& pipeline : queryPlan->getPipelix()) {
        if (pipeline->isOperatorPipeline()) {
            apply(pipeline);
        }
    }
    return queryPlan;
}

std::string getPipelineProviderIdentifier(const QueryCompilation::QueryCompilerOptionsPtr& compilerOptions) {
    switch (compilerOptions->getNautilusBackend()) {
        case QueryCompilerOptions::NautilusBackend::INTERPRETER: {
            return "PipelineInterpreter";
        };
        case QueryCompilerOptions::NautilusBackend::MLIR_COMPILER: {
            return "PipelineCompiler";
        };
        case QueryCompilerOptions::NautilusBackend::BC_INTERPRETER: {
            return "BCInterpreter";
        };
        case QueryCompilerOptions::NautilusBackend::CPP_COMPILER: {
            return "CPPPipelineCompiler";
        };
        default: {
            x_THROW_RUNTIME_ERROR("No pipeline compiler implemented for this backend");
        }
    }
}

OperatorPipelinePtr NautilusCompilationPhase::apply(OperatorPipelinePtr pipeline) {
    auto pipelineRoots = pipeline->getQueryPlan()->getRootOperators();
    x_ASSERT(pipelineRoots.size() == 1, "A pipeline should have a single root operator.");
    auto rootOperator = pipelineRoots[0];
    auto nautilusPipeline = rootOperator->as<NautilusPipelineOperator>();
    Nautilus::CompilationOptions options;
    auto identifier = fmt::format("NautilusCompilation-{}-{}-{}",
                                  pipeline->getQueryPlan()->getQueryId(),
                                  pipeline->getQueryPlan()->getQuerySubPlanId(),
                                  pipeline->getPipelineId());
    options.setIdentifier(identifier);

    // enable dump to console if the compiler options are set
    options.setDumpToConsole(compilerOptions->getDumpMode() == QueryCompilerOptions::DumpMode::CONSOLE
                             || compilerOptions->getDumpMode() == QueryCompilerOptions::DumpMode::FILE_AND_CONSOLE);

    // enable dump to file if the compiler options are set
    options.setDumpToFile(compilerOptions->getDumpMode() == QueryCompilerOptions::DumpMode::FILE
                          || compilerOptions->getDumpMode() == QueryCompilerOptions::DumpMode::FILE_AND_CONSOLE);

    options.setProxyInlining(compilerOptions->getCompilationStrategy()
                             == QueryCompilerOptions::CompilationStrategy::PROXY_INLINING);

    options.setCUDASdkPath(compilerOptions->getCUDASdkPath());

    auto providerName = getPipelineProviderIdentifier(compilerOptions);
    auto& provider = Runtime::Execution::ExecutablePipelineProviderRegistry::getPlugin(providerName);
    auto pipelixtage = provider->create(nautilusPipeline->getNautilusPipeline(), options);
    // we replace the current pipeline operators with an executable operator.
    // this allows us to keep the pipeline structure.
    auto executableOperator = ExecutableOperator::create(std::move(pipelixtage), nautilusPipeline->getOperatorHandlers());
    pipeline->getQueryPlan()->replaceRootOperator(rootOperator, executableOperator);
    return pipeline;
}

}// namespace x::QueryCompilation