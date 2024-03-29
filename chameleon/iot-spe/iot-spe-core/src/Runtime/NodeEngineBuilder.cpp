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
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Compiler/CPPCompiler/CPPCompiler.hpp>
#include <Compiler/JITCompilerBuilder.hpp>
#include <Compiler/LanguageCompiler.hpp>
#include <Components/xWorker.hpp>
#include <Exceptions/SignalHandling.hpp>
#include <Network/NetworkManager.hpp>
#include <Network/PartitionManager.hpp>
#include <QueryCompiler/DefaultQueryCompiler.hpp>
#include <QueryCompiler/NautilusQueryCompiler.hpp>
#include <QueryCompiler/Phases/DefaultPhaseFactory.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/HardwareManager.hpp>
#include <Runtime/MaterializedViewManager.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/NodeEngineBuilder.hpp>
#include <Runtime/OpenCLManager.hpp>
#include <Runtime/QueryManager.hpp>
#include <Util/Common.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <memory>

namespace x::Runtime {

NodeEngineBuilder::NodeEngineBuilder(Configurations::WorkerConfigurationPtr workerConfiguration)
    : workerConfiguration(workerConfiguration) {}

NodeEngineBuilder NodeEngineBuilder::create(Configurations::WorkerConfigurationPtr workerConfiguration) {
    return NodeEngineBuilder(workerConfiguration);
}

NodeEngineBuilder& NodeEngineBuilder::setQueryStatusListener(AbstractQueryStatusListenerPtr xWorker) {
    this->xWorker = std::move(xWorker);
    return *this;
};

NodeEngineBuilder& NodeEngineBuilder::setNodeEngineId(uint64_t nodeEngineId) {
    this->nodeEngineId = nodeEngineId;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setPartitionManager(Network::PartitionManagerPtr partitionManager) {
    this->partitionManager = partitionManager;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setHardwareManager(HardwareManagerPtr hardwareManager) {
    this->hardwareManager = hardwareManager;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setBufferManagers(std::vector<BufferManagerPtr> bufferManagers) {
    this->bufferManagers = bufferManagers;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setQueryManager(QueryManagerPtr queryManager) {
    this->queryManager = queryManager;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setStateManager(StateManagerPtr stateManager) {
    this->stateManager = stateManager;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setMaterializedViewManager(
    x::Experimental::MaterializedView::MaterializedViewManagerPtr materializedViewManager) {
    this->materializedViewManager = materializedViewManager;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setLanguageCompiler(std::shared_ptr<Compiler::LanguageCompiler> languageCompiler) {
    this->languageCompiler = languageCompiler;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setJITCompiler(Compiler::JITCompilerPtr jitCompiler) {
    this->jitCompiler = jitCompiler;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setPhaseFactory(QueryCompilation::Phases::PhaseFactoryPtr phaseFactory) {
    this->phaseFactory = phaseFactory;
    return *this;
}

NodeEngineBuilder& NodeEngineBuilder::setOpenCLManager(x::Runtime::OpenCLManagerPtr openCLManager) {
    this->openCLManager = openCLManager;
    return *this;
}

x::Runtime::NodeEnginePtr NodeEngineBuilder::build() {
    x_ASSERT(xWorker, "xWorker is null");
    try {
        auto nodeEngineId = (this->nodeEngineId == 0) ? getNextNodeEngineId() : this->nodeEngineId;
        auto partitionManager =
            (!this->partitionManager) ? std::make_shared<Network::PartitionManager>() : this->partitionManager;
        auto hardwareManager = (!this->hardwareManager) ? std::make_shared<Runtime::HardwareManager>() : this->hardwareManager;
        std::vector<BufferManagerPtr> bufferManagers;

        //get the list of queue where to pin from the config
        auto numberOfQueues = workerConfiguration->numberOfQueues.getValue();

        //create one buffer manager per queue
        if (numberOfQueues == 1) {
            bufferManagers.push_back(
                std::make_shared<BufferManager>(workerConfiguration->bufferSizeInBytes.getValue(),
                                                workerConfiguration->numberOfBuffersInGlobalBufferManager.getValue(),
                                                hardwareManager->getGlobalAllocator()));
        } else {
            for (auto i = 0u; i < numberOfQueues; ++i) {
                bufferManagers.push_back(std::make_shared<BufferManager>(
                    workerConfiguration->bufferSizeInBytes.getValue(),
                    //if we run in static with multiple queues, we divide the whole buffer manager among the queues
                    workerConfiguration->numberOfBuffersInGlobalBufferManager.getValue() / numberOfQueues,
                    hardwareManager->getGlobalAllocator()));
            }
        }

        if (bufferManagers.empty()) {
            x_ERROR("Runtime: error while building NodeEngine: no xWorker provided");
            throw Exceptions::RuntimeException("Error while building NodeEngine : no xWorker provided",
                                               x::collectAndPrintStacktrace());
        }

        auto stateManager = (!this->stateManager) ? std::make_shared<StateManager>(nodeEngineId) : this->stateManager;

        QueryManagerPtr queryManager{this->queryManager};
        if (!this->queryManager) {
            auto numOfThreads = static_cast<uint16_t>(workerConfiguration->numWorkerThreads.getValue());
            auto numberOfBuffersPerEpoch = static_cast<uint16_t>(workerConfiguration->numberOfBuffersPerEpoch.getValue());
            std::vector<uint64_t> workerToCoreMappingVec =
                x::Util::splitWithStringDelimiter<uint64_t>(workerConfiguration->workerPinList.getValue(), ",");
            switch (workerConfiguration->queryManagerMode.getValue()) {
                case QueryExecutionMode::Dynamic: {
                    queryManager = std::make_shared<DynamicQueryManager>(xWorker,
                                                                         bufferManagers,
                                                                         nodeEngineId,
                                                                         numOfThreads,
                                                                         hardwareManager,
                                                                         stateManager,
                                                                         numberOfBuffersPerEpoch,
                                                                         workerToCoreMappingVec);
                    break;
                }
                case QueryExecutionMode::Static: {
                    queryManager =
                        std::make_shared<MultiQueueQueryManager>(xWorker,
                                                                 bufferManagers,
                                                                 nodeEngineId,
                                                                 numOfThreads,
                                                                 hardwareManager,
                                                                 stateManager,
                                                                 numberOfBuffersPerEpoch,
                                                                 workerToCoreMappingVec,
                                                                 workerConfiguration->numberOfQueues.getValue(),
                                                                 workerConfiguration->numberOfThreadsPerQueue.getValue());
                    break;
                }
                default: {
                    x_ASSERT(false, "Cannot build Query Manager");
                }
            }
        }
        auto materializedViewManager = (!this->materializedViewManager)
            ? std::make_shared<x::Experimental::MaterializedView::MaterializedViewManager>()
            : this->materializedViewManager;
        if (!partitionManager) {
            x_ERROR("Runtime: error while building NodeEngine: error while creating PartitionManager");
            throw Exceptions::RuntimeException("Error while building NodeEngine : Error while creating PartitionManager",
                                               x::collectAndPrintStacktrace());
        }
        if (!queryManager) {
            x_ERROR("Runtime: error while building NodeEngine: error while creating QueryManager");
            throw Exceptions::RuntimeException("Error while building NodeEngine : Error while creating QueryManager",
                                               x::collectAndPrintStacktrace());
        }
        if (!stateManager) {
            x_ERROR("Runtime: error while building NodeEngine: error while creating StateManager");
            throw Exceptions::RuntimeException("Error while building NodeEngine : Error while creating StateManager",
                                               x::collectAndPrintStacktrace());
        }
        if (!materializedViewManager) {
            x_ERROR("Runtime: error while building NodeEngine: error while creating MaterializedViewManager");
            throw Exceptions::RuntimeException("Error while building NodeEngine : error while creating MaterializedViewManager",
                                               x::collectAndPrintStacktrace());
        }

        auto queryCompilationOptions = createQueryCompilationOptions(workerConfiguration->queryCompiler);

        auto phaseFactory = (!this->phaseFactory) ? QueryCompilation::Phases::DefaultPhaseFactory::create() : this->phaseFactory;
        queryCompilationOptions->setNumSourceLocalBuffers(workerConfiguration->numberOfBuffersInSourceLocalBufferPool.getValue());
        QueryCompilation::QueryCompilerPtr compiler;
        if (workerConfiguration->queryCompiler.queryCompilerType
            == QueryCompilation::QueryCompilerOptions::QueryCompiler::DEFAULT_QUERY_COMPILER) {
            auto cppCompiler = (!this->languageCompiler) ? Compiler::CPPCompiler::create() : this->languageCompiler;
            auto jitCompiler = (!this->jitCompiler)
                ? Compiler::JITCompilerBuilder()
                      .registerLanguageCompiler(cppCompiler)
                      .setUseCompilationCache(workerConfiguration->queryCompiler.useCompilationCache.getValue())
                      .build()
                : this->jitCompiler;
            compiler = QueryCompilation::DefaultQueryCompiler::create(queryCompilationOptions,
                                                                      phaseFactory,
                                                                      jitCompiler,
                                                                      workerConfiguration->enableSourceSharing.getValue());
        } else if (workerConfiguration->queryCompiler.queryCompilerType
                   == QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER) {
            compiler = QueryCompilation::NautilusQueryCompiler::create(queryCompilationOptions,
                                                                       phaseFactory,
                                                                       workerConfiguration->enableSourceSharing.getValue());
        }
        if (!compiler) {
            x_ERROR("Runtime: error while building NodeEngine: error while creating compiler");
            throw Exceptions::RuntimeException("Error while building NodeEngine : failed to create compiler",
                                               x::collectAndPrintStacktrace());
        }
        std::vector<PhysicalSourcePtr> physicalSources;
        for (auto entry : workerConfiguration->physicalSources.getValues()) {
            physicalSources.push_back(entry.getValue());
        }
        if (!openCLManager) {
            x_DEBUG("Creating default OpenCLManager");
            openCLManager = std::make_shared<OpenCLManager>();
        }
        std::shared_ptr<NodeEngine> engine = std::make_shared<NodeEngine>(
            physicalSources,
            std::move(hardwareManager),
            std::move(bufferManagers),
            std::move(queryManager),
            [this](const std::shared_ptr<NodeEngine>& engine) {
                return Network::NetworkManager::create(engine->getNodeEngineId(),
                                                       this->workerConfiguration->localWorkerIp.getValue(),
                                                       this->workerConfiguration->dataPort.getValue(),
                                                       Network::ExchangeProtocol(engine->getPartitionManager(), engine),
                                                       engine->getBufferManager(),
                                                       this->workerConfiguration->senderHighwatermark.getValue(),
                                                       this->workerConfiguration->numWorkerThreads.getValue());
            },
            std::move(partitionManager),
            std::move(compiler),
            std::move(stateManager),
            std::move(xWorker),
            std::move(materializedViewManager),
            std::move(openCLManager),
            nodeEngineId,
            workerConfiguration->numberOfBuffersInGlobalBufferManager.getValue(),
            workerConfiguration->numberOfBuffersInSourceLocalBufferPool.getValue(),
            workerConfiguration->numberOfBuffersPerWorker.getValue(),
            workerConfiguration->enableSourceSharing.getValue());
        //        Exceptions::installGlobalErrorListener(engine);
        return engine;
    } catch (std::exception& err) {
        x_ERROR("Cannot start node engine {}", err.what());
        x_THROW_RUNTIME_ERROR("Cant start node engine");
    }
}

QueryCompilation::QueryCompilerOptionsPtr
NodeEngineBuilder::createQueryCompilationOptions(const Configurations::QueryCompilerConfiguration& queryCompilerConfiguration) {
    auto queryCompilerType = queryCompilerConfiguration.queryCompilerType;
    auto queryCompilationOptions = QueryCompilation::QueryCompilerOptions::createDefaultOptions();

    // set compilation mode
    queryCompilationOptions->setCompilationStrategy(queryCompilerConfiguration.compilationStrategy);

    // set pipelining strategy mode
    queryCompilationOptions->setPipeliningStrategy(queryCompilerConfiguration.pipeliningStrategy);

    // set output buffer optimization level
    queryCompilationOptions->setOutputBufferOptimizationLevel(queryCompilerConfiguration.outputBufferOptimizationLevel);

    if (queryCompilerType == QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER
        && queryCompilerConfiguration.windowingStrategy == QueryCompilation::QueryCompilerOptions::WindowingStrategy::LEGACY) {
        // sets SLICING windowing strategy as the default if nautilus is active.
        x_WARNING("The LEGACY window strategy is not supported by Nautilus. Switch to SLICING!")
        queryCompilationOptions->setWindowingStrategy(QueryCompilation::QueryCompilerOptions::WindowingStrategy::SLICING);
    } else {
        // sets the windowing strategy
        queryCompilationOptions->setWindowingStrategy(queryCompilerConfiguration.windowingStrategy);
    }

    // sets the query compiler
    queryCompilationOptions->setQueryCompiler(queryCompilerConfiguration.queryCompilerType);

    // set the dump mode
    queryCompilationOptions->setDumpMode(queryCompilerConfiguration.queryCompilerDumpMode);

    // set nautilus backend
    queryCompilationOptions->setNautilusBackend(queryCompilerConfiguration.nautilusBackend);

    queryCompilationOptions->getHashJoinOptions()->setNumberOfPartitions(
        queryCompilerConfiguration.numberOfPartitions.getValue());
    queryCompilationOptions->getHashJoinOptions()->setPageSize(queryCompilerConfiguration.pageSize.getValue());
    queryCompilationOptions->getHashJoinOptions()->setPreAllocPageCnt(queryCompilerConfiguration.preAllocPageCnt.getValue());
    //zero indicate that it has not been set in the yaml config
    if (queryCompilerConfiguration.maxHashTableSize.getValue() != 0) {
        queryCompilationOptions->getHashJoinOptions()->setTotalSizeForDataStructures(
            queryCompilerConfiguration.maxHashTableSize.getValue());
    }

    queryCompilationOptions->setStreamJoinStratgy(queryCompilerConfiguration.joinStrategy);

    queryCompilationOptions->setCUDASdkPath(queryCompilerConfiguration.cudaSdkPath.getValue());

    return queryCompilationOptions;
}

uint64_t NodeEngineBuilder::getNextNodeEngineId() {
    const uint64_t max = -1;
    uint64_t id = time(nullptr) ^ getpid();
    return (++id % max);
}
}//namespace x::Runtime