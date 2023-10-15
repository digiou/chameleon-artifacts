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
#include <API/QueryAPI.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MemorySourceType.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Services/TopologyManagerService.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>
#include <filesystem>
#include <type_traits>
#include <utility>
namespace x {

TestHarxs::TestHarxs(std::string queryWithoutSink,
                         uint16_t restPort,
                         uint16_t rpcPort,
                         std::filesystem::path testHarxsResourcePath,
                         uint64_t memSrcFrequency,
                         uint64_t memSrcNumBuffToProcess)
    : queryWithoutSinkStr(std::move(queryWithoutSink)), coordinatorIPAddress("127.0.0.1"), restPort(restPort), rpcPort(rpcPort),
      useNautilus(false), performDistributedWindowOptimization(false), useNewRequestExecutor(false),
      memSrcFrequency(memSrcFrequency), memSrcNumBuffToProcess(memSrcNumBuffToProcess), bufferSize(4096), physicalSourceCount(0),
      topologyId(1), joinStrategy(QueryCompilation::StreamJoinStrategy::xTED_LOOP_JOIN), validationDone(false),
      topologySetupDone(false), testHarxsResourcePath(testHarxsResourcePath) {}

TestHarxs::TestHarxs(Query queryWithoutSink,
                         uint16_t restPort,
                         uint16_t rpcPort,
                         std::filesystem::path testHarxsResourcePath,
                         uint64_t memSrcFrequency,
                         uint64_t memSrcNumBuffToProcess)
    : queryWithoutSinkStr(""), queryWithoutSink(std::make_shared<Query>(std::move(queryWithoutSink))),
      coordinatorIPAddress("127.0.0.1"), restPort(restPort), rpcPort(rpcPort), useNautilus(false),
      performDistributedWindowOptimization(false), useNewRequestExecutor(false), memSrcFrequency(memSrcFrequency),
      memSrcNumBuffToProcess(memSrcNumBuffToProcess), bufferSize(4096), physicalSourceCount(0), topologyId(1),
      joinStrategy(QueryCompilation::StreamJoinStrategy::xTED_LOOP_JOIN), validationDone(false), topologySetupDone(false),
      testHarxsResourcePath(testHarxsResourcePath) {}

TestHarxs& TestHarxs::addLogicalSource(const std::string& logicalSourceName, const SchemaPtr& schema) {
    auto logicalSource = LogicalSource::create(logicalSourceName, schema);
    this->logicalSources.emplace_back(logicalSource);
    return *this;
}

TestHarxs& TestHarxs::enableNautilus() {
    useNautilus = true;
    return *this;
}

TestHarxs& TestHarxs::enableDistributedWindowOptimization() {
    performDistributedWindowOptimization = true;
    return *this;
}

TestHarxs& TestHarxs::enableNewRequestExecutor() {
    useNewRequestExecutor = true;
    return *this;
}

TestHarxs& TestHarxs::setJoinStrategy(QueryCompilation::StreamJoinStrategy& newJoinStrategy) {
    this->joinStrategy = newJoinStrategy;
    return *this;
}

void TestHarxs::checkAndAddLogicalSources() {

    for (const auto& logicalSource : logicalSources) {

        auto logicalSourceName = logicalSource->getLogicalSourceName();
        auto schema = logicalSource->getSchema();

        // Check if logical source already exists
        auto sourceCatalog = xCoordinator->getSourceCatalog();
        if (!sourceCatalog->containsLogicalSource(logicalSourceName)) {
            x_TRACE("TestHarxs: logical source does not exist in the source catalog, adding a new logical source {}",
                      logicalSourceName);
            sourceCatalog->addLogicalSource(logicalSourceName, schema);
        } else {
            // Check if it has the same schema
            if (!sourceCatalog->getSchemaForLogicalSource(logicalSourceName)->equals(schema, true)) {
                x_TRACE("TestHarxs: logical source {} exists in the source catalog with different schema, replacing it "
                          "with a new schema",
                          logicalSourceName);
                sourceCatalog->removeLogicalSource(logicalSourceName);
                sourceCatalog->addLogicalSource(logicalSourceName, schema);
            }
        }
    }
}

TestHarxs& TestHarxs::attachWorkerWithMemorySourceToWorkerWithId(const std::string& logicalSourceName,
                                                                     uint32_t parentId,
                                                                     WorkerConfigurationPtr workerConfiguration) {
    workerConfiguration->parentId = parentId;
#ifdef TFDEF
    workerConfiguration->isTensorflowSupported = true;
#endif// TFDEF
    std::string physicalSourceName = getNextPhysicalSourceName();
    auto workerId = getNextTopologyId();
    auto testHarxsWorkerConfiguration =
        TestHarxsWorkerConfiguration::create(workerConfiguration,
                                               logicalSourceName,
                                               physicalSourceName,
                                               TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::MemorySource,
                                               workerId);
    testHarxsWorkerConfigurations.emplace_back(testHarxsWorkerConfiguration);
    return *this;
}

TestHarxs& TestHarxs::attachWorkerWithMemorySourceToCoordinator(const std::string& logicalSourceName) {
    //We are assuming coordinator will start with id 1
    return attachWorkerWithMemorySourceToWorkerWithId(std::move(logicalSourceName), 1);
}

TestHarxs& TestHarxs::attachWorkerWithLambdaSourceToCoordinator(const std::string& logicalSourceName,
                                                                    PhysicalSourceTypePtr physicalSource,
                                                                    WorkerConfigurationPtr workerConfiguration) {
    //We are assuming coordinator will start with id 1
    workerConfiguration->parentId = 1;
    std::string physicalSourceName = getNextPhysicalSourceName();
    auto workerId = getNextTopologyId();
    auto testHarxsWorkerConfiguration =
        TestHarxsWorkerConfiguration::create(workerConfiguration,
                                               logicalSourceName,
                                               physicalSourceName,
                                               TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::LambdaSource,
                                               workerId);
    testHarxsWorkerConfiguration->setPhysicalSourceType(physicalSource);
    testHarxsWorkerConfigurations.emplace_back(testHarxsWorkerConfiguration);
    return *this;
}

TestHarxs& TestHarxs::attachWorkerWithCSVSourceToWorkerWithId(const std::string& logicalSourceName,
                                                                  CSVSourceTypePtr csvSourceType,
                                                                  uint64_t parentId) {
    auto workerConfiguration = WorkerConfiguration::create();
    std::string physicalSourceName = getNextPhysicalSourceName();
    auto physicalSource = PhysicalSource::create(logicalSourceName, physicalSourceName, csvSourceType);
    workerConfiguration->physicalSources.add(physicalSource);
    workerConfiguration->parentId = parentId;
    uint32_t workerId = getNextTopologyId();
    auto testHarxsWorkerConfiguration =
        TestHarxsWorkerConfiguration::create(workerConfiguration,
                                               logicalSourceName,
                                               physicalSourceName,
                                               TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::CSVSource,
                                               workerId);
    testHarxsWorkerConfigurations.emplace_back(testHarxsWorkerConfiguration);
    return *this;
}

TestHarxs& TestHarxs::attachWorkerWithCSVSourceToCoordinator(const std::string& logicalSourceName,
                                                                 const CSVSourceTypePtr& csvSourceType) {
    //We are assuming coordinator will start with id 1
    return attachWorkerWithCSVSourceToWorkerWithId(std::move(logicalSourceName), std::move(csvSourceType), 1);
}

TestHarxs& TestHarxs::attachWorkerToWorkerWithId(uint32_t parentId) {

    auto workerConfiguration = WorkerConfiguration::create();
    workerConfiguration->parentId = parentId;
    uint32_t workerId = getNextTopologyId();
    auto testHarxsWorkerConfiguration = TestHarxsWorkerConfiguration::create(workerConfiguration, workerId);
    testHarxsWorkerConfigurations.emplace_back(testHarxsWorkerConfiguration);
    return *this;
}

TestHarxs& TestHarxs::attachWorkerToCoordinator() {
    //We are assuming coordinator will start with id 1
    return attachWorkerToWorkerWithId(1);
}
uint64_t TestHarxs::getWorkerCount() { return testHarxsWorkerConfigurations.size(); }

TestHarxs& TestHarxs::validate() {
    validationDone = true;
    if (this->logicalSources.empty()) {
        throw Exceptions::RuntimeException(
            "No Logical source defined. Please make sure you add logical source while defining up test harxs.");
    }

    if (testHarxsWorkerConfigurations.empty()) {
        throw Exceptions::RuntimeException("TestHarxs: No worker added to the test harxs.");
    }

    uint64_t sourceCount = 0;
    for (const auto& workerConf : testHarxsWorkerConfigurations) {
        if (workerConf->getSourceType() == TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::MemorySource
            && workerConf->getRecords().empty()) {
            throw Exceptions::RuntimeException("TestHarxs: No Record defined for Memory Source with logical source Name: "
                                               + workerConf->getLogicalSourceName() + " and Physical source name : "
                                               + workerConf->getPhysicalSourceName() + ". Please add data to the test harxs.");
        }

        if (workerConf->getSourceType() == TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::CSVSource
            || workerConf->getSourceType() == TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::MemorySource
            || workerConf->getSourceType() == TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::LambdaSource) {
            sourceCount++;
        }
    }

    if (sourceCount == 0) {
        throw Exceptions::RuntimeException("TestHarxs: No Physical source defined in the test harxs.");
    }
    return *this;
}

PhysicalSourcePtr TestHarxs::createPhysicalSourceOfLambdaType(TestHarxsWorkerConfigurationPtr workerConf) {
    // create and populate memory source
    auto currentSourceNumOfRecords = workerConf->getRecords().size();

    auto logicalSourceName = workerConf->getLogicalSourceName();

    SchemaPtr schema;
    for (const auto& logicalSource : logicalSources) {
        if (logicalSource->getLogicalSourceName() == logicalSourceName) {
            schema = logicalSource->getSchema();
        }
    }

    if (!schema) {
        throw Exceptions::RuntimeException("Unable to find logical source with name " + logicalSourceName
                                           + ". Make sure you are adding a logical source with the name to the test harxs.");
    }

    return PhysicalSource::create(logicalSourceName, workerConf->getPhysicalSourceName(), workerConf->getPhysicalSourceType());
};

PhysicalSourcePtr TestHarxs::createPhysicalSourceOfMemoryType(TestHarxsWorkerConfigurationPtr workerConf) {
    // create and populate memory source
    auto currentSourceNumOfRecords = workerConf->getRecords().size();

    auto logicalSourceName = workerConf->getLogicalSourceName();

    SchemaPtr schema;
    for (const auto& logicalSource : logicalSources) {
        if (logicalSource->getLogicalSourceName() == logicalSourceName) {
            schema = logicalSource->getSchema();
        }
    }

    if (!schema) {
        throw Exceptions::RuntimeException("Unable to find logical source with name " + logicalSourceName
                                           + ". Make sure you are adding a logical source with the name to the test harxs.");
    }

    auto tupleSize = schema->getSchemaSizeInBytes();
    x_DEBUG("Tuple Size: {}", tupleSize);
    x_DEBUG("currentSourceNumOfRecords: {}", currentSourceNumOfRecords);
    auto memAreaSize = currentSourceNumOfRecords * tupleSize;
    auto* memArea = reinterpret_cast<uint8_t*>(malloc(memAreaSize));

    auto currentRecords = workerConf->getRecords();
    for (std::size_t j = 0; j < currentSourceNumOfRecords; ++j) {
        memcpy(&memArea[tupleSize * j], currentRecords.at(j), tupleSize);
    }

    x_ASSERT2_FMT(bufferSize >= schema->getSchemaSizeInBytes() * currentSourceNumOfRecords,
                    "TestHarxs: A record might span multiple buffers and this is not supported bufferSize="
                        << bufferSize << " recordSize=" << schema->getSchemaSizeInBytes());
    auto memorySourceType =
        MemorySourceType::create(memArea, memAreaSize, memSrcNumBuffToProcess, memSrcFrequency, GatheringMode::INTERVAL_MODE);
    return PhysicalSource::create(logicalSourceName, workerConf->getPhysicalSourceName(), memorySourceType);
};

TestHarxs& TestHarxs::setupTopology(std::function<void(CoordinatorConfigurationPtr)> crdConfigFunctor) {
    if (!validationDone) {
        x_THROW_RUNTIME_ERROR("Please call validate before calling setup.");
    }

    //Start Coordinator
    auto coordinatorConfiguration = CoordinatorConfiguration::createDefault();
    coordinatorConfiguration->coordinatorIp = coordinatorIPAddress;
    coordinatorConfiguration->restPort = restPort;
    coordinatorConfiguration->rpcPort = rpcPort;

    if (useNewRequestExecutor) {
        coordinatorConfiguration->enableNewRequestExecutor = true;
    }

    if (useNautilus) {
        coordinatorConfiguration->worker.queryCompiler.queryCompilerType =
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
        coordinatorConfiguration->worker.queryCompiler.queryCompilerDumpMode =
            QueryCompilation::QueryCompilerOptions::DumpMode::CONSOLE;
        coordinatorConfiguration->worker.queryCompiler.joinStrategy = joinStrategy;
        coordinatorConfiguration->optimizer.performDistributedWindowOptimization = performDistributedWindowOptimization;

        // Only this is currently supported in Nautilus
        coordinatorConfiguration->worker.queryCompiler.windowingStrategy =
            QueryCompilation::QueryCompilerOptions::WindowingStrategy::SLICING;
    }
    crdConfigFunctor(coordinatorConfiguration);

    xCoordinator = std::make_shared<xCoordinator>(coordinatorConfiguration);
    auto coordinatorRPCPort = xCoordinator->startCoordinator(/**blocking**/ false);
    //Add all logical sources
    checkAndAddLogicalSources();

    std::vector<TopologyNodeId> workerIds;

    for (auto& workerConf : testHarxsWorkerConfigurations) {

        //Fetch the worker configuration
        auto workerConfiguration = workerConf->getWorkerConfiguration();
        if (useNautilus) {
            workerConfiguration->queryCompiler.queryCompilerType =
                QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
            workerConfiguration->queryCompiler.queryCompilerDumpMode = QueryCompilation::QueryCompilerOptions::DumpMode::CONSOLE;

            // Only this is currently supported in Nautilus
            workerConfiguration->queryCompiler.windowingStrategy =
                QueryCompilation::QueryCompilerOptions::WindowingStrategy::SLICING;
            workerConfiguration->queryCompiler.joinStrategy = joinStrategy;
        }

        //Set ports at runtime
        workerConfiguration->coordinatorPort = coordinatorRPCPort;
        workerConfiguration->coordinatorIp = coordinatorIPAddress;

        switch (workerConf->getSourceType()) {
            case TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::MemorySource: {
                auto physicalSource = createPhysicalSourceOfMemoryType(workerConf);
                workerConfiguration->physicalSources.add(physicalSource);
                break;
            }
            case TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::LambdaSource: {
                auto physicalSource = createPhysicalSourceOfLambdaType(workerConf);
                workerConfiguration->physicalSources.add(physicalSource);
                break;
            }
            default: break;
        }

        xWorkerPtr xWorker = std::make_shared<xWorker>(std::move(workerConfiguration));
        xWorker->start(/**blocking**/ false, /**withConnect**/ true);
        workerIds.emplace_back(xWorker->getTopologyNodeId());

        //We are assuming that coordinator has a node id 1
        xWorker->replaceParent(1, xWorker->getWorkerConfiguration()->parentId.getValue());

        //Add x Worker to the configuration.
        //Note: this is required to stop the xWorker at the end of the test
        workerConf->setQueryStatusListener(xWorker);
    }

    auto topologyManagerService = xCoordinator->getTopologyManagerService();

    auto start_timestamp = std::chrono::system_clock::now();

    for (const auto& workerId : workerIds) {
        while (!topologyManagerService->findNodeWithId(workerId)) {
            if (std::chrono::system_clock::now() > start_timestamp + SETUP_TIMEOUT_IN_SEC) {
                x_THROW_RUNTIME_ERROR("TestHarxs: Unable to find setup topology in given timeout.");
            }
        }
    }
    topologySetupDone = true;
    return *this;
}

TopologyPtr TestHarxs::getTopology() {

    if (!validationDone && !topologySetupDone) {
        throw Exceptions::RuntimeException(
            "Make sure to call first validate() and then setupTopology() to the test harxs before checking the output");
    }
    return xCoordinator->getTopology();
};

const QueryPlanPtr& TestHarxs::getQueryPlan() const { return queryPlan; }

std::string TestHarxs::getNextPhysicalSourceName() {
    physicalSourceCount++;
    return std::to_string(physicalSourceCount);
}

uint32_t TestHarxs::getNextTopologyId() {
    topologyId++;
    return topologyId;
}

}// namespace x