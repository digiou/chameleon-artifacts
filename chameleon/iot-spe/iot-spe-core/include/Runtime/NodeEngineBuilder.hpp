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

#ifndef x_CORE_INCLUDE_RUNTIME_NODEENGINEBUILDER_HPP_
#define x_CORE_INCLUDE_RUNTIME_NODEENGINEBUILDER_HPP_
#include <Network/NetworkForwardRefs.hpp>
#include <QueryCompiler/QueryCompilerForwardDeclaration.hpp>
#include <Runtime/RuntimeForwardRefs.hpp>

namespace x {
namespace Compiler {
class LanguageCompiler;

class JITCompiler;
using JITCompilerPtr = std::shared_ptr<JITCompiler>;
}// namespace Compiler
namespace Configurations {
class QueryCompilerConfiguration;
}
namespace Experimental::MaterializedView {
class MaterializedViewManager;
using MaterializedViewManagerPtr = std::shared_ptr<MaterializedViewManager>;
}// namespace Experimental::MaterializedView

}// namespace x
namespace x::Runtime {
/**
 * This class is used to create instances of NodeEngine using the builder pattern.
 */
class NodeEngineBuilder {

  public:
    NodeEngineBuilder() = delete;
    /**
     * Creates a default NodeEngineBuilder
     * @param workerConfiguration contains values that configure aspects of the NodeEngine
     * @return NodeEngineBuilder
     */
    static NodeEngineBuilder create(Configurations::WorkerConfigurationPtr workerConfiguration);

    /**
     * setter used to pass xWorker to NodeEngineBuilder. Optional
     * @param xWorker
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setQueryStatusListener(AbstractQueryStatusListenerPtr xWorker);

    /**
     * setter used to pass a NodeEngineId to NodeEngineBuilder. Optional
     * @param nodeEngineId
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setNodeEngineId(uint64_t nodeEngineId);

    /**
     * setter used to pass a partition manager to NodeEngineBuilder. Optional
     * @param partitionManager
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setPartitionManager(Network::PartitionManagerPtr partitionManager);

    /**
     * setter used to pass a hardware manager to NodeEngineBuilder. Optional
     * @param hardwareManager
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setHardwareManager(HardwareManagerPtr hardwareManager);

    /**
     * setter used to pass a vector of buffer managers to NodeEngineBuilder. Optional
     * @param std::vector<BufferManagerPtr>
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setBufferManagers(std::vector<BufferManagerPtr> bufferManagers);

    /**
     * setter used to pass a query manager to NodeEngineBuilder. Optional
     * @param queryManager
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setQueryManager(QueryManagerPtr queryManager);

    /**
     * setter used to pass a state manager to NodeEngineBuilder. Optional
     * @param stateManager
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setStateManager(StateManagerPtr stateManager);

    /**
     * setter used to pass a materialized view manager to NodeEngineBuilder. Optional
     * @param materializedViewManager
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder&
    setMaterializedViewManager(x::Experimental::MaterializedView::MaterializedViewManagerPtr materializedViewManager);

    /**
     * setter used to pass a language compiler to NodeEngineBuilder. Optional
     * @param languageCompiler
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setLanguageCompiler(std::shared_ptr<Compiler::LanguageCompiler> languageCompiler);

    /**
     * setter used to pass a language a jit compiler to NodeEngineBuilder. Optional
     * @param jitCompiler
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setJITCompiler(Compiler::JITCompilerPtr jitCompiler);

    /**
     * setter used to pass a language a phase factory to NodeEngineBuilder. Optional
     * @param phaseFactory
     * @return NodeEngineBuilder&
     */
    NodeEngineBuilder& setPhaseFactory(QueryCompilation::Phases::PhaseFactoryPtr phaseFactory);

    /**
     * setter used to pass an OpenCL manager to NodeEngineBuilder. Optional.
     */
    NodeEngineBuilder& setOpenCLManager(OpenCLManagerPtr openCLManager);

    /**
     * performs safety checks and returns a NodeEngine
     * @return NodeEnginePtr
     */
    NodeEnginePtr build();

  private:
    explicit NodeEngineBuilder(Configurations::WorkerConfigurationPtr workerConfiguration);

    std::shared_ptr<AbstractQueryStatusListener> xWorker;
    uint64_t nodeEngineId = 0;
    Network::PartitionManagerPtr partitionManager;
    HardwareManagerPtr hardwareManager;
    std::vector<BufferManagerPtr> bufferManagers;
    QueryManagerPtr queryManager;
    StateManagerPtr stateManager;
    x::Experimental::MaterializedView::MaterializedViewManagerPtr materializedViewManager;
    std::shared_ptr<Compiler::LanguageCompiler> languageCompiler;
    Compiler::JITCompilerPtr jitCompiler;
    QueryCompilation::Phases::PhaseFactoryPtr phaseFactory;
    QueryCompilation::QueryCompilerPtr queryCompiler;
    Configurations::WorkerConfigurationPtr workerConfiguration;
    OpenCLManagerPtr openCLManager;

    /**
     *  Used during build() to convert the QueryCompilerConfigurations in the WorkerConfigruations to QueryCompilationOptions,
     *  which is then used to create a QueryCompiler
     * @param queryCompilerConfiguration : values to confiugre
     * @return QueryCompilerOptionsPtr
     */
    static QueryCompilation::QueryCompilerOptionsPtr
    createQueryCompilationOptions(const Configurations::QueryCompilerConfiguration& queryCompilerConfiguration);

    /**
     * @brief Returns the next free node id
     * @return node id
     */
    static uint64_t getNextNodeEngineId();
};
}// namespace x::Runtime
#endif// x_CORE_INCLUDE_RUNTIME_NODEENGINEBUILDER_HPP_
