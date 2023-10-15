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
#include <QueryCompiler/CodeGenerator/CCodeGenerator/CCodeGenerator.hpp>
#include <QueryCompiler/Phases/AddScanAndEmitPhase.hpp>
#include <QueryCompiler/Phases/BufferOptimizationPhase.hpp>
#include <QueryCompiler/Phases/CodeGenerationPhase.hpp>
#include <QueryCompiler/Phases/DefaultPhaseFactory.hpp>
#include <QueryCompiler/Phases/PhaseFactory.hpp>
#include <QueryCompiler/Phases/Pipelining/DefaultPipeliningPhase.hpp>
#include <QueryCompiler/Phases/Pipelining/FuseNonPipelineBreakerPolicy.hpp>
#include <QueryCompiler/Phases/Pipelining/OperatorAtATimePolicy.hpp>
#include <QueryCompiler/Phases/PredicationOptimizationPhase.hpp>
#include <QueryCompiler/Phases/Translations/DataSinkProvider.hpp>
#include <QueryCompiler/Phases/Translations/DefaultDataSourceProvider.hpp>
#include <QueryCompiler/Phases/Translations/DefaultGeneratableOperatorProvider.hpp>
#include <QueryCompiler/Phases/Translations/DefaultPhysicalOperatorProvider.hpp>
#include <QueryCompiler/Phases/Translations/LowerLogicalToPhysicalOperators.hpp>
#include <QueryCompiler/Phases/Translations/LowerPhysicalToGeneratableOperators.hpp>
#include <QueryCompiler/Phases/Translations/LowerToExecutableQueryPlanPhase.hpp>
#include <QueryCompiler/Phases/Translations/SourceSharingDataSourceProvider.hpp>
#include <QueryCompiler/QueryCompilerOptions.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::QueryCompilation::Phases {

PhaseFactoryPtr DefaultPhaseFactory::create() { return std::make_shared<DefaultPhaseFactory>(); }

PipeliningPhasePtr DefaultPhaseFactory::createPipeliningPhase(QueryCompilerOptionsPtr options) {
    switch (options->getPipeliningStrategy()) {
        case QueryCompilerOptions::PipeliningStrategy::OPERATOR_FUSION: {
            x_DEBUG("Create pipelining phase with fuse policy");
            auto operatorFusionPolicy = FuseNonPipelineBreakerPolicy::create();
            return DefaultPipeliningPhase::create(operatorFusionPolicy);
        };
        case QueryCompilerOptions::PipeliningStrategy::OPERATOR_AT_A_TIME: {
            x_DEBUG("Create pipelining phase with always break policy");
            auto operatorFusionPolicy = OperatorAtATimePolicy::create();
            return DefaultPipeliningPhase::create(operatorFusionPolicy);
        }
    };
}

LowerLogicalToPhysicalOperatorsPtr DefaultPhaseFactory::createLowerLogicalQueryPlanPhase(QueryCompilerOptionsPtr options) {
    x_DEBUG("Create default lower logical plan phase");
    auto physicalOperatorProvider = DefaultPhysicalOperatorProvider::create(options);
    return LowerLogicalToPhysicalOperators::create(physicalOperatorProvider);
}

AddScanAndEmitPhasePtr DefaultPhaseFactory::createAddScanAndEmitPhase(QueryCompilerOptionsPtr) {
    x_DEBUG("Create add scan and emit phase");
    return AddScanAndEmitPhase::create();
}
LowerPhysicalToGeneratableOperatorsPtr
DefaultPhaseFactory::createLowerPhysicalToGeneratableOperatorsPhase(QueryCompilerOptionsPtr) {
    x_DEBUG("Create default lower pipeline plan phase");
    auto generatableOperatorProvider = DefaultGeneratableOperatorProvider::create();
    return LowerPhysicalToGeneratableOperators::create(generatableOperatorProvider);
}
CodeGenerationPhasePtr DefaultPhaseFactory::createCodeGenerationPhase(QueryCompilerOptionsPtr options,
                                                                      Compiler::JITCompilerPtr jitCompiler) {
    x_DEBUG("Create default code generation phase");
    // TODO create a option to choose between different code generators.
    auto codeGenerator = CCodeGenerator::create();
    return CodeGenerationPhase::create(codeGenerator, jitCompiler, options->getCompilationStrategy());
}
LowerToExecutableQueryPlanPhasePtr DefaultPhaseFactory::createLowerToExecutableQueryPlanPhase(QueryCompilerOptionsPtr options,
                                                                                              bool sourceSharing) {
    x_DEBUG("Create lower to executable query plan phase");
    DataSourceProviderPtr sourceProvider;
    if (!sourceSharing) {
        sourceProvider = DefaultDataSourceProvider::create(options);
    } else {
        sourceProvider = SourceSharingDataSourceProvider::create(options);
    }

    auto sinkProvider = DataSinkProvider::create();
    return LowerToExecutableQueryPlanPhase::create(sinkProvider, sourceProvider);
}
BufferOptimizationPhasePtr DefaultPhaseFactory::createBufferOptimizationPhase(QueryCompilerOptionsPtr options) {
    x_DEBUG("Create buffer optimization phase");
    return BufferOptimizationPhase::create(options->getOutputBufferOptimizationLevel());
}
PredicationOptimizationPhasePtr DefaultPhaseFactory::createPredicationOptimizationPhase(QueryCompilerOptionsPtr options) {
    x_DEBUG("Create predication optimization phase");
    return PredicationOptimizationPhase::create(options->getFilterProcessingStrategy());
}
}// namespace x::QueryCompilation::Phases