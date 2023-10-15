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
#include <BaseIntegrationTest.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/DefaultSourceType.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <Catalogs/UDF/UDFCatalog.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Compiler/CPPCompiler/CPPCompiler.hpp>
#include <Compiler/JITCompilerBuilder.hpp>
#include <Configurations/Worker/QueryCompilerConfiguration.hpp>
#include <Network/NetworkChannel.hpp>
#include <Network/NetworkManager.hpp>
#include <Network/NetworkSink.hpp>
#include <Network/NetworkSource.hpp>
#include <Network/PartitionManager.hpp>
#include <Network/ZmqServer.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <QueryCompiler/DefaultQueryCompiler.hpp>
#include <QueryCompiler/Phases/DefaultPhaseFactory.hpp>
#include <QueryCompiler/QueryCompilationRequest.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/HardwareManager.hpp>
#include <Runtime/MaterializedViewManager.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/MemoryLayout/RowLayoutField.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/NodeEngineBuilder.hpp>
#include <Runtime/OpenCLManager.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/RuntimeForwardRefs.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Sinks/Formats/xFormat.hpp>
#include <Sinks/Mediums/NullOutputSink.hpp>
#include <Sources/DefaultSource.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestQuery.hpp>
#include <Util/TestQueryCompiler.hpp>
#include <Util/TestSinkDescriptor.hpp>
#include <Util/TestSourceDescriptor.hpp>
#include <Util/TestUtils.hpp>
#include <Util/ThreadBarrier.hpp>
#include <gtest/gtest.h>
#include <random>
#include <utility>

using namespace std;

namespace x {
using Runtime::TupleBuffer;
class QueryParsingService;
using QueryParsingServicePtr = std::shared_ptr<QueryParsingService>;
const uint64_t buffersManaged = 8 * 1024;
const uint64_t bufferSize = 32 * 1024;

struct TestStruct {
    int64_t id;
    int64_t one;
    int64_t value;
};

static constexpr auto NSOURCE_RETRIES = 100;
static constexpr auto NSOURCE_RETRY_WAIT = std::chrono::milliseconds(5);

namespace Network {
class NetworkStackIntegrationTest : public Testing::BaseIntegrationTest {
  public:
    Catalogs::UDF::UDFCatalogPtr udfCatalog;
    Catalogs::Source::SourceCatalogPtr sourceCatalog;
    static void SetUpTestCase() {
        x::Logger::setupLogging("NetworkStackIntegrationTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("SetUpTestCase NetworkStackIntegrationTest");
    }

    void SetUp() {
        Testing::BaseIntegrationTest::SetUp();
        dataPort1 = Testing::BaseIntegrationTest::getAvailablePort();
        dataPort2 = Testing::BaseIntegrationTest::getAvailablePort();
        sourceCatalog = std::make_shared<Catalogs::Source::SourceCatalog>(QueryParsingServicePtr());
        udfCatalog = Catalogs::UDF::UDFCatalog::create();
    }

    void TearDown() {
        dataPort1.reset();
        dataPort2.reset();
        Testing::BaseIntegrationTest::TearDown();
    }

    static void TearDownTestCase() { x_INFO("TearDownTestCase NetworkStackIntegrationTest."); }

  protected:
    Testing::BorrowedPortPtr dataPort1;
    Testing::BorrowedPortPtr dataPort2;
};

class TestSink : public SinkMedium {
  public:
    SinkMediumTypes getSinkMediumType() override { return SinkMediumTypes::PRINT_SINK; }

    TestSink(const SchemaPtr& schema,
             Runtime::NodeEnginePtr nodeEngine,
             const Runtime::BufferManagerPtr& bufferManager,
             uint32_t numOfProducers = 1,
             QueryId queryId = 0,
             QuerySubPlanId querySubPlanId = 0)
        : SinkMedium(std::make_shared<xFormat>(schema, bufferManager), nodeEngine, numOfProducers, queryId, querySubPlanId) {
        // nop
    }

    bool writeData(Runtime::TupleBuffer& input_buffer, Runtime::WorkerContextRef) override {
        std::unique_lock lock(m);

        auto rowLayout = Runtime::MemoryLayouts::RowLayout::create(getSchemaPtr(), input_buffer.getBufferSize());
        auto dynamicTupleBuffer = Runtime::MemoryLayouts::DynamicTupleBuffer(rowLayout, input_buffer);
        std::stringstream dynamicTupleBufferAsString;
        dynamicTupleBufferAsString << dynamicTupleBuffer;
        x_TRACE("TestSink:\n{}", dynamicTupleBufferAsString.str());

        uint64_t sum = 0;
        for (uint64_t i = 0; i < input_buffer.getNumberOfTuples(); ++i) {
            sum += input_buffer.getBuffer<TestStruct>()[i].value;
        }

        completed.set_value(sum);
        return true;
    }

    std::string toString() const override { return ""; }

    void setup() override{};

    void shutdown() override{};

    ~TestSink() override = default;

    std::mutex m;
    std::promise<uint64_t> completed;
};

void fillBuffer(TupleBuffer& buf, const Runtime::MemoryLayouts::RowLayoutPtr& memoryLayout) {
    auto recordIndexFields = Runtime::MemoryLayouts::RowLayoutField<int64_t, true>::create(0, memoryLayout, buf);
    auto fields01 = Runtime::MemoryLayouts::RowLayoutField<int64_t, true>::create(1, memoryLayout, buf);
    auto fields02 = Runtime::MemoryLayouts::RowLayoutField<int64_t, true>::create(2, memoryLayout, buf);

    for (int recordIndex = 0; recordIndex < 10; recordIndex++) {
        recordIndexFields[recordIndex] = recordIndex;
        fields01[recordIndex] = 1;
        fields02[recordIndex] = recordIndex % 2;
    }
    buf.setNumberOfTuples(10);
}

template<typename MockedNodeEngine, typename... ExtraParameters>
std::shared_ptr<MockedNodeEngine> createMockedEngine(const std::string& hostname,
                                                     uint16_t port,
                                                     uint64_t bufferSize,
                                                     uint64_t numBuffers,
                                                     ExtraParameters&&... extraParams) {
    try {
        class DummyQueryListener : public AbstractQueryStatusListener {
          public:
            virtual ~DummyQueryListener() {}

            bool canTriggerEndOfStream(QueryId, QuerySubPlanId, OperatorId, Runtime::QueryTerminationType) override {
                return true;
            }
            bool notifySourceTermination(QueryId, QuerySubPlanId, OperatorId, Runtime::QueryTerminationType) override {
                return true;
            }
            bool notifyQueryFailure(QueryId, QuerySubPlanId, std::string) override { return true; }
            bool notifyQueryStatusChange(QueryId, QuerySubPlanId, Runtime::Execution::ExecutableQueryPlanStatus) override {
                return true;
            }
            bool notifyEpochTermination(uint64_t, uint64_t) override { return false; }
        };
        auto defaultSourceType = DefaultSourceType::create();
        auto physicalSource = PhysicalSource::create("default_logical", "default", defaultSourceType);
        std::vector<PhysicalSourcePtr> physicalSources{physicalSource};
        auto partitionManager = std::make_shared<Network::PartitionManager>();
        auto stateManager = std::make_shared<Runtime::StateManager>(0);
        std::vector<Runtime::BufferManagerPtr> bufferManagers = {
            std::make_shared<Runtime::BufferManager>(bufferSize, numBuffers)};
        auto hwManager = std::make_shared<Runtime::HardwareManager>();
        auto queryManager = std::make_shared<Runtime::DynamicQueryManager>(std::make_shared<DummyQueryListener>(),
                                                                           bufferManagers,
                                                                           0,
                                                                           1,
                                                                           hwManager,
                                                                           stateManager,
                                                                           100);
        auto networkManagerCreator = [=](const Runtime::NodeEnginePtr& engine) {
            return Network::NetworkManager::create(0,
                                                   hostname,
                                                   port,
                                                   Network::ExchangeProtocol(partitionManager, engine),
                                                   bufferManagers[0]);
        };
        auto cppCompiler = Compiler::CPPCompiler::create();
        auto jitCompiler = Compiler::JITCompilerBuilder().registerLanguageCompiler(cppCompiler).build();
        auto phaseFactory = QueryCompilation::Phases::DefaultPhaseFactory::create();
        auto options = QueryCompilation::QueryCompilerOptions::createDefaultOptions();
        options->setNumSourceLocalBuffers(12);

        auto compiler = QueryCompilation::DefaultQueryCompiler::create(options, phaseFactory, jitCompiler);

        return std::make_shared<MockedNodeEngine>(std::move(physicalSources),
                                                  std::move(hwManager),
                                                  std::move(bufferManagers),
                                                  std::move(queryManager),
                                                  std::move(networkManagerCreator),
                                                  std::move(partitionManager),
                                                  std::move(compiler),
                                                  std::forward<ExtraParameters>(extraParams)...);

    } catch (std::exception& err) {
        x_ERROR("Cannot start node engine {}", err.what());
        x_THROW_RUNTIME_ERROR("Cant start node engine");
        return nullptr;
    }
}

TEST_F(NetworkStackIntegrationTest, testNetworkSourceSink) {
    std::promise<bool> completed;
    atomic<int> bufferCnt = 0;
    uint64_t totalNumBuffer = 100;

    static constexpr int numSendingThreads = 4;
    auto sendingThreads = std::vector<std::thread>();
    auto schema = Schema::create()->addField("id", DataTypeFactory::createInt64());

    NodeLocation nodeLocationSource{0, "127.0.0.1", *dataPort1};
    NodeLocation nodeLocationSink{0, "127.0.0.1", *dataPort2};

    xPartition xPartition{1, 22, 33, 44};
    ThreadBarrierPtr sinkShutdownBarrier = std::make_shared<ThreadBarrier>(numSendingThreads + 1);

    class MockedNodeEngine : public Runtime::NodeEngine {
      public:
        xPartition xPartition;
        std::promise<bool>& completed;
        atomic<int> eosCnt = 0;
        atomic<int>& bufferCnt;

        explicit MockedNodeEngine(std::vector<PhysicalSourcePtr> physicalSources,
                                  Runtime::HardwareManagerPtr hardwareManager,
                                  std::vector<x::Runtime::BufferManagerPtr>&& bufferManagers,
                                  x::Runtime::QueryManagerPtr&& queryManager,
                                  std::function<Network::NetworkManagerPtr(x::Runtime::NodeEnginePtr)>&& networkManagerCreator,
                                  Network::PartitionManagerPtr&& partitionManager,
                                  QueryCompilation::QueryCompilerPtr&& queryCompiler,
                                  std::promise<bool>& completed,
                                  xPartition xPartition,
                                  std::atomic<int>& bufferCnt)
            : NodeEngine(std::move(physicalSources),
                         std::move(hardwareManager),
                         std::move(bufferManagers),
                         std::move(queryManager),
                         std::move(networkManagerCreator),
                         std::move(partitionManager),
                         std::move(queryCompiler),
                         std::make_shared<x::Runtime::StateManager>(0),
                         std::make_shared<DummyQueryListener>(),
                         std::make_shared<x::Experimental::MaterializedView::MaterializedViewManager>(),
                         std::make_shared<x::Runtime::OpenCLManager>(),
                         0,
                         64,
                         64,
                         12,
                         false),
              xPartition(xPartition), completed(completed), bufferCnt(bufferCnt) {}

        ~MockedNodeEngine() = default;

        void onDataBuffer(Network::xPartition id, TupleBuffer&) override {
            if (xPartition == id) {
                bufferCnt++;
            }
            ASSERT_EQ(id, xPartition);
        }

        void onEndOfStream(Network::Messages::EndOfStreamMessage) override {
            eosCnt++;
            if (eosCnt == 1) {
                completed.set_value(true);
            }
        }

        void onServerError(Network::Messages::ErrorMessage ex) override {
            if (ex.getErrorType() != Messages::ErrorType::PartitionNotRegisteredError) {
                completed.set_exception(make_exception_ptr(runtime_error("Error")));
            }
        }

        void onChannelError(Network::Messages::ErrorMessage message) override { NodeEngine::onChannelError(message); }
    };

    try {
        std::thread receivingThread([&]() {
            auto recvEngine = createMockedEngine<MockedNodeEngine>("127.0.0.1",
                                                                   *dataPort1,
                                                                   bufferSize,
                                                                   buffersManaged,
                                                                   completed,
                                                                   xPartition,
                                                                   bufferCnt);
            // register the incoming channel
            auto sink = std::make_shared<NullOutputSink>(recvEngine, 1, 0, 0);
            std::vector<Runtime::Execution::SuccessorExecutablePipeline> succ = {sink};
            auto source = std::make_shared<NetworkSource>(schema,
                                                          recvEngine->getBufferManager(),
                                                          recvEngine->getQueryManager(),
                                                          recvEngine->getNetworkManager(),
                                                          xPartition,
                                                          nodeLocationSink,
                                                          64,
                                                          NSOURCE_RETRY_WAIT,
                                                          NSOURCE_RETRIES,
                                                          std::move(succ));
            auto qep = Runtime::Execution::ExecutableQueryPlan::create(0,
                                                                       0,
                                                                       {source},
                                                                       {sink},
                                                                       {},
                                                                       recvEngine->getQueryManager(),
                                                                       recvEngine->getBufferManager());
            recvEngine->getQueryManager()->registerQuery(qep);
            ASSERT_EQ(recvEngine->getPartitionManager()->getConsumerRegistrationStatus(xPartition),
                      PartitionRegistrationStatus::Registered);
            completed.get_future().get();
            sinkShutdownBarrier->wait();
            while (!(qep->getStatus() == Runtime::Execution::ExecutableQueryPlanStatus::Stopped
                     || qep->getStatus() == Runtime::Execution::ExecutableQueryPlanStatus::Finished)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            ASSERT_TRUE(recvEngine->stop());
        });

        auto defaultSourceType = DefaultSourceType::create();
        auto physicalSource = PhysicalSource::create("default_logical", "default", defaultSourceType);
        auto workerConfig2 = WorkerConfiguration::create();
        workerConfig2->dataPort = *dataPort2;
        workerConfig2->bufferSizeInBytes = bufferSize;
        workerConfig2->physicalSources.add(physicalSource);
        auto nodeEngineBuilder2 =
            Runtime::NodeEngineBuilder::create(workerConfig2).setQueryStatusListener(std::make_shared<DummyQueryListener>());
        auto sendEngine = nodeEngineBuilder2.build();

        auto networkSink = std::make_shared<
            NetworkSink>(schema, 0, 0, 0, nodeLocationSource, xPartition, sendEngine, 1, NSOURCE_RETRY_WAIT, NSOURCE_RETRIES);
        networkSink->preSetup();
        for (int threadNr = 0; threadNr < numSendingThreads; threadNr++) {
            std::thread sendingThread([&] {
                // register the incoming channel
                Runtime::WorkerContext workerContext(Runtime::xThread::getId(), sendEngine->getBufferManager(), 64);
                auto rt = Runtime::ReconfigurationMessage(0,
                                                          0,
                                                          Runtime::ReconfigurationType::Initialize,
                                                          networkSink,
                                                          std::make_any<uint32_t>(1));
                networkSink->reconfigure(rt, workerContext);
                for (uint64_t i = 0; i < totalNumBuffer; ++i) {
                    auto buffer = sendEngine->getBufferManager()->getBufferBlocking();
                    for (uint64_t j = 0; j < bufferSize / sizeof(uint64_t); ++j) {
                        buffer.getBuffer<uint64_t>()[j] = j;
                    }
                    buffer.setNumberOfTuples(bufferSize / sizeof(uint64_t));
                    networkSink->writeData(buffer, workerContext);
                    usleep(rand() % 10000 + 1000);
                }
                auto rt2 = Runtime::ReconfigurationMessage(0, 0, Runtime::ReconfigurationType::SoftEndOfStream, networkSink);
                networkSink->reconfigure(rt2, workerContext);
                sinkShutdownBarrier->wait();
            });
            sendingThreads.emplace_back(std::move(sendingThread));
        }

        for (std::thread& t : sendingThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        networkSink->shutdown();
        receivingThread.join();
        ASSERT_TRUE(sendEngine->stop());
    } catch (...) {
        FAIL();
    }
    auto const bf = bufferCnt.load();
    ASSERT_TRUE(bf > 0);
    ASSERT_EQ(static_cast<std::size_t>(bf), numSendingThreads * totalNumBuffer);
}

/**
 * @brief this test triggers buffering and turns it of again while tuples are ingested into a network source.
 * It verifies if all buffered tuples are unbuffered properly and arrive at the receiving side
 */
TEST_F(NetworkStackIntegrationTest, testReconnectBufferingSink) {
    std::promise<bool> completed;
    atomic<int> bufferCnt = 0;
    uint64_t totalNumBuffer = 100;

    static constexpr int numSendingThreads = 4;
    auto sendingThreads = std::vector<std::thread>();
    auto sendingContexts = std::vector<Runtime::WorkerContext>();
    auto schema = Schema::create()->addField("id", DataTypeFactory::createInt64());

    NodeLocation nodeLocationSource{0, "127.0.0.1", *dataPort1};
    NodeLocation nodeLocationSink{0, "127.0.0.1", *dataPort2};

    xPartition xPartition{1, 22, 33, 44};
    ThreadBarrierPtr waitBeforeBufferBarrier = std::make_shared<ThreadBarrier>(numSendingThreads + 1);
    ThreadBarrierPtr sinkShutdownBarrier = std::make_shared<ThreadBarrier>(numSendingThreads + 1);

    class MockedNodeEngine : public Runtime::NodeEngine {
      public:
        xPartition xPartition;
        std::promise<bool>& completed;
        atomic<int> eosCnt = 0;
        atomic<int>& bufferCnt;

        explicit MockedNodeEngine(std::vector<PhysicalSourcePtr> physicalSources,
                                  Runtime::HardwareManagerPtr hardwareManager,
                                  std::vector<x::Runtime::BufferManagerPtr>&& bufferManagers,
                                  x::Runtime::QueryManagerPtr&& queryManager,
                                  std::function<Network::NetworkManagerPtr(x::Runtime::NodeEnginePtr)>&& networkManagerCreator,
                                  Network::PartitionManagerPtr&& partitionManager,
                                  QueryCompilation::QueryCompilerPtr&& queryCompiler,
                                  std::promise<bool>& completed,
                                  xPartition xPartition,
                                  std::atomic<int>& bufferCnt)
            : NodeEngine(std::move(physicalSources),
                         std::move(hardwareManager),
                         std::move(bufferManagers),
                         std::move(queryManager),
                         std::move(networkManagerCreator),
                         std::move(partitionManager),
                         std::move(queryCompiler),
                         std::make_shared<x::Runtime::StateManager>(0),
                         std::make_shared<DummyQueryListener>(),
                         std::make_shared<x::Experimental::MaterializedView::MaterializedViewManager>(),
                         std::make_shared<x::Runtime::OpenCLManager>(),
                         0,
                         64,
                         64,
                         12,
                         false),
              xPartition(xPartition), completed(completed), bufferCnt(bufferCnt) {}

        ~MockedNodeEngine() = default;

        void onDataBuffer(Network::xPartition id, TupleBuffer&) override {
            if (xPartition == id) {
                bufferCnt++;
            }
            ASSERT_EQ(id, xPartition);
        }

        void onEndOfStream(Network::Messages::EndOfStreamMessage) override {
            eosCnt++;
            if (eosCnt == 1) {
                completed.set_value(true);
            }
        }

        void onServerError(Network::Messages::ErrorMessage ex) override {
            if (ex.getErrorType() != Messages::ErrorType::PartitionNotRegisteredError) {
                completed.set_exception(make_exception_ptr(runtime_error("Error")));
            }
        }

        void onChannelError(Network::Messages::ErrorMessage message) override { NodeEngine::onChannelError(message); }
    };

    try {
        std::thread receivingThread([&]() {
            auto recvEngine = createMockedEngine<MockedNodeEngine>("127.0.0.1",
                                                                   *dataPort1,
                                                                   bufferSize,
                                                                   buffersManaged,
                                                                   completed,
                                                                   xPartition,
                                                                   bufferCnt);
            // register the incoming channel
            auto sink = std::make_shared<NullOutputSink>(recvEngine, 1, 0, 0);
            std::vector<Runtime::Execution::SuccessorExecutablePipeline> succ = {sink};
            auto source = std::make_shared<NetworkSource>(schema,
                                                          recvEngine->getBufferManager(),
                                                          recvEngine->getQueryManager(),
                                                          recvEngine->getNetworkManager(),
                                                          xPartition,
                                                          nodeLocationSink,
                                                          64,
                                                          NSOURCE_RETRY_WAIT,
                                                          NSOURCE_RETRIES,
                                                          std::move(succ));
            auto qep = Runtime::Execution::ExecutableQueryPlan::create(0,
                                                                       0,
                                                                       {source},
                                                                       {sink},
                                                                       {},
                                                                       recvEngine->getQueryManager(),
                                                                       recvEngine->getBufferManager());
            recvEngine->getQueryManager()->registerQuery(qep);
            ASSERT_EQ(recvEngine->getPartitionManager()->getConsumerRegistrationStatus(xPartition),
                      PartitionRegistrationStatus::Registered);
            completed.get_future().get();
            sinkShutdownBarrier->wait();
            while (!(qep->getStatus() == Runtime::Execution::ExecutableQueryPlanStatus::Stopped
                     || qep->getStatus() == Runtime::Execution::ExecutableQueryPlanStatus::Finished)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            ASSERT_TRUE(recvEngine->stop());
        });

        auto defaultSourceType = DefaultSourceType::create();
        auto physicalSource = PhysicalSource::create("default_logical", "default", defaultSourceType);
        auto workerConfig2 = WorkerConfiguration::create();
        workerConfig2->dataPort = *dataPort2;
        workerConfig2->bufferSizeInBytes = bufferSize;
        workerConfig2->physicalSources.add(physicalSource);
        auto nodeEngineBuilder2 =
            Runtime::NodeEngineBuilder::create(workerConfig2).setQueryStatusListener(std::make_shared<DummyQueryListener>());
        auto sendEngine = nodeEngineBuilder2.build();

        auto networkSink = std::make_shared<
            NetworkSink>(schema, 0, 0, 0, nodeLocationSource, xPartition, sendEngine, 1, NSOURCE_RETRY_WAIT, NSOURCE_RETRIES);
        networkSink->preSetup();
        for (int threadNr = 0; threadNr < numSendingThreads; threadNr++) {
            std::thread sendingThread([&] {
                // register the incoming channel
                Runtime::WorkerContext workerContext(Runtime::xThread::getId(), sendEngine->getBufferManager(), 64);
                auto rt = Runtime::ReconfigurationMessage(0,
                                                          0,
                                                          Runtime::ReconfigurationType::Initialize,
                                                          networkSink,
                                                          std::make_any<uint32_t>(1));
                networkSink->reconfigure(rt, workerContext);
                for (uint64_t i = 0; i < totalNumBuffer; ++i) {
                    auto buffer = sendEngine->getBufferManager()->getBufferBlocking();
                    for (uint64_t j = 0; j < bufferSize / sizeof(uint64_t); ++j) {
                        buffer.getBuffer<uint64_t>()[j] = j;
                    }
                    buffer.setNumberOfTuples(bufferSize / sizeof(uint64_t));
                    //make threads buffer here
                    networkSink->writeData(buffer, workerContext);
                    usleep(rand() % 10000 + 1000);
                }
                waitBeforeBufferBarrier->wait();
                auto rt2 = Runtime::ReconfigurationMessage(0, 0, Runtime::ReconfigurationType::SoftEndOfStream, networkSink);
                networkSink->reconfigure(rt2, workerContext);
                sinkShutdownBarrier->wait();
            });
            sendingThreads.emplace_back(std::move(sendingThread));
        }

        auto prevCount = bufferCnt.load();
        while (static_cast<std::size_t>(bufferCnt.load()) < numSendingThreads * totalNumBuffer / 2) {
            if (bufferCnt.load() != prevCount) {
                prevCount = bufferCnt.load();
                x_DEBUG("Count before buffer: {}", prevCount);
            }
        }
        auto bufferReconfigMsg = Runtime::ReconfigurationMessage(0,
                                                                 0,
                                                                 Runtime::ReconfigurationType::StartBuffering,
                                                                 networkSink,
                                                                 std::make_any<uint32_t>(1));

        sendEngine->bufferAllData();
        sleep(1);
        auto lastBufferCnt = bufferCnt.load();
        for (int i = 0; i < 10; ++i) {
            x_DEBUG("Count while buffering: {}", bufferCnt.load());
            EXPECT_EQ(lastBufferCnt, bufferCnt.load());
            sleep(1);
        }
        sendEngine->stopBufferingAllData();
        x_DEBUG("Count after buffer: {}", bufferCnt.load());
        waitBeforeBufferBarrier->wait();

        for (std::thread& t : sendingThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        networkSink->shutdown();
        receivingThread.join();
        ASSERT_TRUE(sendEngine->stop());
    } catch (...) {
        FAIL();
    }
    auto const bf = bufferCnt.load();
    ASSERT_TRUE(bf > 0);
    ASSERT_EQ(static_cast<std::size_t>(bf), numSendingThreads * totalNumBuffer);
}

namespace detail {
class TestSourceWithLatch : public DefaultSource {
  public:
    explicit TestSourceWithLatch(const SchemaPtr& schema,
                                 const Runtime::BufferManagerPtr& bufferManager,
                                 const Runtime::QueryManagerPtr& queryManager,
                                 OperatorId operatorId,
                                 size_t numSourceLocalBuffers,
                                 const std::vector<Runtime::Execution::SuccessorExecutablePipeline>& successors,
                                 ThreadBarrierPtr latch)
        : DefaultSource(schema,
                        bufferManager,
                        queryManager,
                        /*bufferCnt*/ 1,
                        /*frequency*/ 0,
                        operatorId,
                        /*oridingid*/ 0,
                        numSourceLocalBuffers,
                        successors),
          latch(std::move(latch)) {}

    void runningRoutine() override {
        latch->wait();
        DataSource::runningRoutine();
    }

  private:
    ThreadBarrierPtr latch;
};
}// namespace detail

TEST_F(NetworkStackIntegrationTest, testQEPNetworkSinkSource) {

    auto numQueries = 10;
    auto numThreads = 8;
    SchemaPtr schema = Schema::create()
                           ->addField("test$id", DataTypeFactory::createInt64())
                           ->addField("test$one", DataTypeFactory::createInt64())
                           ->addField("test$value", DataTypeFactory::createInt64());

    auto defaultSourceType = DefaultSourceType::create();
    std::vector<PhysicalSourcePtr> physicalSources;
    for (auto i = 0; i < numQueries; ++i) {
        auto str = std::to_string(i);
        auto physicalSource = PhysicalSource::create("default_logical"s + str, "default"s + str, defaultSourceType);
        physicalSources.emplace_back(physicalSource);
    }

    auto latch = std::make_shared<ThreadBarrier>(numQueries);

    auto workerConfiguration1 = WorkerConfiguration::create();
    workerConfiguration1->dataPort.setValue(*dataPort1);
    for (auto source : physicalSources) {
        workerConfiguration1->physicalSources.add(source);
    }
    workerConfiguration1->numWorkerThreads.setValue(numThreads);
    workerConfiguration1->bufferSizeInBytes.setValue(bufferSize);
    workerConfiguration1->numberOfBuffersInGlobalBufferManager.setValue(buffersManaged);
    workerConfiguration1->numberOfBuffersInSourceLocalBufferPool.setValue(64);
    workerConfiguration1->numberOfBuffersPerWorker.setValue(12);

    auto nodeEngixender = Runtime::NodeEngineBuilder::create(workerConfiguration1)
                                .setQueryStatusListener(std::make_shared<DummyQueryListener>())
                                .build();
    auto netManagerSender = nodeEngixender->getNetworkManager();
    NodeLocation nodeLocationSender = netManagerSender->getServerLocation();
    auto workerConfiguration2 = WorkerConfiguration::create();
    workerConfiguration2->dataPort.setValue(*dataPort2);
    for (auto source : physicalSources) {
        workerConfiguration2->physicalSources.add(source);
    }
    workerConfiguration2->numWorkerThreads.setValue(numThreads);
    workerConfiguration2->bufferSizeInBytes.setValue(bufferSize);
    workerConfiguration2->numberOfBuffersInGlobalBufferManager.setValue(buffersManaged);
    workerConfiguration2->numberOfBuffersInSourceLocalBufferPool.setValue(64);
    workerConfiguration2->numberOfBuffersPerWorker.setValue(12);
    auto nodeEngineReceiver = Runtime::NodeEngineBuilder::create(workerConfiguration2)
                                  .setQueryStatusListener(std::make_shared<DummyQueryListener>())
                                  .build();

    auto netManagerReceiver = nodeEngineReceiver->getNetworkManager();
    NodeLocation nodeLocationReceiver = netManagerReceiver->getServerLocation();

    std::vector<std::shared_ptr<TestSink>> finalSinks;

    uint32_t subPlanId = 0;
    for (auto i = 1; i <= numQueries; ++i) {
        xPartition xPartition{x::QueryId(i),
                                  x::OperatorId(i * 22),
                                  x::PartitionId(i * 33),
                                  x::SubpartitionId(i * 44)};
        // create NetworkSink
        auto networkSourceDescriptor1 = std::make_shared<TestUtils::TestSourceDescriptor>(
            schema,
            [&](SchemaPtr schema,
                OperatorId,
                OriginId,
                const SourceDescriptorPtr&,
                const Runtime::NodeEnginePtr&,
                size_t numSourceLocalBuffers,
                const std::vector<Runtime::Execution::SuccessorExecutablePipeline>& successors) -> DataSourcePtr {
                return std::make_shared<NetworkSource>(schema,
                                                       nodeEngineReceiver->getBufferManager(),
                                                       nodeEngineReceiver->getQueryManager(),
                                                       netManagerReceiver,
                                                       xPartition,
                                                       nodeLocationSender,
                                                       numSourceLocalBuffers,
                                                       NSOURCE_RETRY_WAIT,
                                                       NSOURCE_RETRIES,
                                                       successors);
            });

        auto testSink =
            std::make_shared<TestSink>(schema, nodeEngineReceiver, nodeEngineReceiver->getBufferManager(), 1, i, subPlanId);
        finalSinks.emplace_back(testSink);
        auto testSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(testSink);

        auto query = TestQuery::from(networkSourceDescriptor1).sink(testSinkDescriptor);

        auto typeInferencePhase = Optimizer::TypeInferencePhase::create(sourceCatalog, udfCatalog);
        auto queryPlan = typeInferencePhase->execute(query.getQueryPlan());
        queryPlan->setQueryId(i);
        queryPlan->setQuerySubPlanId(subPlanId++);
        auto request = QueryCompilation::QueryCompilationRequest::create(queryPlan, nodeEngineReceiver);
        auto queryCompiler = TestUtils::createTestQueryCompiler();
        auto result = queryCompiler->compileQuery(request);
        auto builderReceiverQEP = result->getExecutableQueryPlan();

        // creating query plan
        auto testSourceDescriptor = std::make_shared<TestUtils::TestSourceDescriptor>(
            schema,
            [&](SchemaPtr schema,
                OperatorId,
                OriginId,
                const SourceDescriptorPtr&,
                const Runtime::NodeEnginePtr&,
                size_t numSourceLocalBuffers,
                std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors) -> DataSourcePtr {
                return std::make_shared<detail::TestSourceWithLatch>(schema,
                                                                     nodeEngixender->getBufferManager(),
                                                                     nodeEngixender->getQueryManager(),
                                                                     OperatorId(1 + i),
                                                                     numSourceLocalBuffers,
                                                                     std::move(successors),
                                                                     latch);
            });

        auto networkSink = std::make_shared<NetworkSink>(schema,
                                                         i,
                                                         i,
                                                         subPlanId,
                                                         nodeLocationReceiver,
                                                         xPartition,
                                                         nodeEngixender,
                                                         1,
                                                         NSOURCE_RETRY_WAIT,
                                                         NSOURCE_RETRIES);
        auto networkSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(networkSink);
        auto query2 = TestQuery::from(testSourceDescriptor).filter(Attribute("id") < 5).sink(networkSinkDescriptor);

        auto queryPlan2 = typeInferencePhase->execute(query2.getQueryPlan());
        queryPlan2->setQueryId(i);
        queryPlan2->setQuerySubPlanId(subPlanId++);
        auto request2 = QueryCompilation::QueryCompilationRequest::create(queryPlan2, nodeEngixender);
        auto result2 = queryCompiler->compileQuery(request2);
        auto builderGeneratorQEP = result2->getExecutableQueryPlan();
        //        ASSERT_TRUE(nodeEngixender->registerQueryInNodeEngine(builderGeneratorQEP));
        //        ASSERT_TRUE(nodeEngineReceiver->registerQueryInNodeEngine(builderReceiverQEP));
        //        ASSERT_TRUE(nodeEngixender->startQuery(builderGeneratorQEP->getQueryId()));
        //        ASSERT_TRUE(nodeEngineReceiver->startQuery(builderReceiverQEP->getQueryId()));

        auto func = [](auto engine, auto qep) {
            return engine->registerQueryInNodeEngine(qep);
        };

        auto f1 = std::async(std::launch::async, func, nodeEngixender, builderGeneratorQEP);
        auto f2 = std::async(std::launch::async, func, nodeEngineReceiver, builderReceiverQEP);

        ASSERT_TRUE(f1.get());
        ASSERT_TRUE(f2.get());
        ASSERT_TRUE(nodeEngixender->startQuery(builderGeneratorQEP->getQueryId()));
        ASSERT_TRUE(nodeEngineReceiver->startQuery(builderReceiverQEP->getQueryId()));
    }

    ASSERT_EQ(numQueries, finalSinks.size());

    for (const auto& testSink : finalSinks) {
        ASSERT_EQ(10ULL, testSink->completed.get_future().get());
    }

    x_DEBUG("All network sinks are completed");

    while (true) {
        auto completedSubQueries = 0u;
        for (auto i = 1; i <= numQueries; ++i) {
            for (auto engine : {nodeEngineReceiver, nodeEngixender}) {
                auto qepStatus = engine->getQueryStatus(i);
                if (qepStatus == Runtime::Execution::ExecutableQueryPlanStatus::Stopped
                    || qepStatus == Runtime::Execution::ExecutableQueryPlanStatus::Finished) {
                    completedSubQueries++;
                }
            }
        }
        if (completedSubQueries == subPlanId) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    x_DEBUG("All qeps are completed");
    ASSERT_TRUE(nodeEngixender->stop());
    ASSERT_TRUE(nodeEngineReceiver->stop());
}

namespace detail {
struct TestEvent {
    explicit TestEvent(Runtime::EventType ev, uint32_t test) : ev(ev), test(test) {}

    Runtime::EventType getEventType() const { return ev; }

    uint32_t testValue() const { return test; }

    Runtime::EventType ev;
    uint32_t test;
};

}// namespace detail

TEST_F(NetworkStackIntegrationTest, testSendEvent) {
    std::promise<bool> completedProm;

    std::atomic<bool> eventReceived = false;
    auto xPartition = xPartition(1, 22, 333, 444);

    try {

        class ExchangeListener : public ExchangeProtocolListener {

          public:
            std::promise<bool>& completedProm;
            std::atomic<bool>& eventReceived;

            ExchangeListener(std::atomic<bool>& bufferReceived, std::promise<bool>& completedProm)
                : completedProm(completedProm), eventReceived(bufferReceived) {}

            void onDataBuffer(xPartition, TupleBuffer&) override {}

            void onEvent(xPartition, Runtime::BaseEvent& event) override {
                eventReceived = event.getEventType() == Runtime::EventType::kCustomEvent
                    && dynamic_cast<Runtime::CustomEventWrapper&>(event).data<detail::TestEvent>()->testValue() == 123;
                ASSERT_TRUE(eventReceived);
            }
            void onEndOfStream(Messages::EndOfStreamMessage) override { completedProm.set_value(true); }
            void onServerError(Messages::ErrorMessage) override {}

            void onChannelError(Messages::ErrorMessage) override {}
        };

        auto partMgr = std::make_shared<PartitionManager>();
        auto buffMgr = std::make_shared<Runtime::BufferManager>(bufferSize, buffersManaged);

        auto netManager =
            NetworkManager::create(0,
                                   "127.0.0.1",
                                   *dataPort2,
                                   ExchangeProtocol(partMgr, std::make_shared<ExchangeListener>(eventReceived, completedProm)),
                                   buffMgr);

        struct DataEmitterImpl : public DataEmitter {
            void emitWork(TupleBuffer&) override {}
        };
        std::thread t([&netManager, &xPartition, &completedProm, this] {
            // register the incoming channel
            sleep(3);// intended stalling to simulate latency
            auto nodeLocation = NodeLocation(0, "127.0.0.1", *dataPort2);
            netManager->registerSubpartitionConsumer(xPartition, nodeLocation, std::make_shared<DataEmitterImpl>());
            auto future = completedProm.get_future();
            if (future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
                ASSERT_TRUE(future.get());
            } else {
                x_ERROR("NetworkStackIntegrationTest: Receiving thread timed out!");
            }
            netManager->unregisterSubpartitionConsumer(xPartition);
        });

        NodeLocation nodeLocation(0, "127.0.0.1", *dataPort2);
        auto senderChannel =
            netManager->registerSubpartitionProducer(nodeLocation, xPartition, buffMgr, std::chrono::seconds(1), 5);

        if (senderChannel == nullptr) {
            x_INFO("NetworkStackIntegrationTest: Error in registering DataChannel!");
            completedProm.set_value(false);
        } else {
            senderChannel->sendEvent<detail::TestEvent>(Runtime::EventType::kCustomEvent, 123);
            senderChannel->close(Runtime::QueryTerminationType::Graceful);
            senderChannel.reset();
            netManager->unregisterSubpartitionProducer(xPartition);
        }

        t.join();
    } catch (...) {
        FAIL();
    }
    ASSERT_TRUE(eventReceived.load());
}

TEST_F(NetworkStackIntegrationTest, DISABLED_testSendEventBackward) {

    NodeLocation nodeLocationSender{0, "127.0.0.1", *dataPort1};
    NodeLocation nodeLocationReceiver{0, "127.0.0.1", *dataPort2};
    xPartition xPartition{1, 22, 33, 44};
    SchemaPtr schema = Schema::create()
                           ->addField("test$id", DataTypeFactory::createInt64())
                           ->addField("test$one", DataTypeFactory::createInt64())
                           ->addField("test$value", DataTypeFactory::createInt64());
    auto queryCompilerConfiguration = Configurations::QueryCompilerConfiguration();

    auto defaultSourceType = DefaultSourceType::create();
    auto physicalSource = PhysicalSource::create("default_logical", "default", defaultSourceType);
    auto workerConfiguration1 = WorkerConfiguration::create();
    workerConfiguration1->dataPort.setValue(*dataPort1);
    workerConfiguration1->physicalSources.add(physicalSource);
    workerConfiguration1->bufferSizeInBytes.setValue(bufferSize);
    workerConfiguration1->numberOfBuffersInGlobalBufferManager.setValue(buffersManaged);
    workerConfiguration1->numberOfBuffersInSourceLocalBufferPool.setValue(64);
    workerConfiguration1->numberOfBuffersPerWorker.setValue(12);

    auto nodeEngixender = Runtime::NodeEngineBuilder::create(workerConfiguration1)
                                .setQueryStatusListener(std::make_shared<DummyQueryListener>())
                                .build();
    auto workerConfiguration2 = WorkerConfiguration::create();
    workerConfiguration2->dataPort.setValue(*dataPort2);
    workerConfiguration2->physicalSources.add(physicalSource);
    workerConfiguration2->bufferSizeInBytes.setValue(bufferSize);
    workerConfiguration2->numberOfBuffersInGlobalBufferManager.setValue(buffersManaged);
    workerConfiguration2->numberOfBuffersInSourceLocalBufferPool.setValue(64);
    workerConfiguration2->numberOfBuffersPerWorker.setValue(12);

    std::promise<bool> queryCompleted;

    class TestQueryListener : public DummyQueryListener {
      public:
        explicit TestQueryListener(std::promise<bool>& queryCompleted) : queryCompleted(queryCompleted) {}

        bool notifyQueryStatusChange(QueryId id,
                                     QuerySubPlanId planId,
                                     Runtime::Execution::ExecutableQueryPlanStatus status) override {
            queryCompleted.set_value(true);
            return DummyQueryListener::notifyQueryStatusChange(id, planId, status);
        }

      private:
        std::promise<bool>& queryCompleted;
    };

    auto nodeEngineReceiver = Runtime::NodeEngineBuilder::create(workerConfiguration2)
                                  .setQueryStatusListener(std::make_shared<TestQueryListener>(queryCompleted))
                                  .build();
    // create NetworkSink

    class TestNetworkSink : public NetworkSink {
      public:
        using NetworkSink::NetworkSink;

      protected:
        void onEvent(Runtime::BaseEvent& event) override {
            // NetworkSink::onEvent(event);
            bool eventReceived = event.getEventType() == Runtime::EventType::kCustomEvent
                && dynamic_cast<Runtime::CustomEventWrapper&>(event).data<detail::TestEvent>()->testValue() == 123;
            bool expected = false;
            if (sourceNotifier->compare_exchange_strong(expected, true)) {
                completed.set_value(eventReceived);
            }
        }

      public:
        std::promise<bool> completed;
        std::atomic<bool>* sourceNotifier;
    };

    auto networkSourceDescriptor1 = std::make_shared<TestUtils::TestSourceDescriptor>(
        schema,
        [&](SchemaPtr schema,
            OperatorId,
            OriginId,
            const SourceDescriptorPtr&,
            const Runtime::NodeEnginePtr&,
            size_t numSourceLocalBuffers,
            const std::vector<Runtime::Execution::SuccessorExecutablePipeline>& successors) -> DataSourcePtr {
            return std::make_shared<NetworkSource>(schema,
                                                   nodeEngineReceiver->getBufferManager(),
                                                   nodeEngineReceiver->getQueryManager(),
                                                   nodeEngineReceiver->getNetworkManager(),
                                                   xPartition,
                                                   nodeLocationSender,
                                                   numSourceLocalBuffers,
                                                   NSOURCE_RETRY_WAIT,
                                                   NSOURCE_RETRIES,
                                                   successors);
        });

    class TestSourceEvent : public GeneratorSource {
      public:
        explicit TestSourceEvent(const SchemaPtr& schema,
                                 const Runtime::BufferManagerPtr& bufferManager,
                                 const Runtime::QueryManagerPtr& queryManager,
                                 OperatorId operatorId,
                                 size_t numSourceLocalBuffers,
                                 const std::vector<Runtime::Execution::SuccessorExecutablePipeline>& successors)
            : GeneratorSource(schema,
                              bufferManager,
                              queryManager,
                              1000,
                              operatorId,
                              0,
                              numSourceLocalBuffers,
                              GatheringMode::INTERVAL_MODE,
                              successors) {}

        std::optional<Runtime::TupleBuffer> receiveData() override {
            if (!canStop) {
                auto buffer = bufferManager->getBufferBlocking();
                auto* writer = buffer.getBuffer<int64_t>();
                writer[0] = 1;
                writer[1] = 2;
                writer[2] = 3;
                buffer.setNumberOfTuples(1);
                return buffer;
            } else {
                return {};
            }
        }

      public:
        std::atomic<bool> canStop{false};
    };

    class TestSinkEvent : public SinkMedium {
      public:
        SinkMediumTypes getSinkMediumType() override { return SinkMediumTypes::PRINT_SINK; }

        explicit TestSinkEvent(const SchemaPtr& schema,
                               Runtime::NodeEnginePtr nodeEngine,
                               const Runtime::BufferManagerPtr& bufferManager,
                               uint32_t numOfProducers = 1,
                               QueryId queryId = 0,
                               QuerySubPlanId querySubPlanId = 0)
            : SinkMedium(std::make_shared<xFormat>(schema, bufferManager),
                         nodeEngine,
                         numOfProducers,
                         queryId,
                         querySubPlanId) {
            // nop
        }

        bool writeData(Runtime::TupleBuffer&, Runtime::WorkerContextRef context) override {
            auto parentPlan = nodeEngine->getQueryManager()->getQueryExecutionPlan(querySubPlanId);
            for (auto& dataSources : parentPlan->getSources()) {
                auto senderChannel = context.getEventOnlyNetworkChannel(dataSources->getOperatorId());
                senderChannel->sendEvent<detail::TestEvent>(Runtime::EventType::kCustomEvent, 123);
            }
            return true;
        }

        void setup() override {}
        void shutdown() override {}
        string toString() const override { return std::string(); }
    };

    auto testSink = std::make_shared<TestSinkEvent>(schema, nodeEngineReceiver, nodeEngineReceiver->getBufferManager());
    auto testSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(testSink);

    auto query = TestQuery::from(networkSourceDescriptor1).sink(testSinkDescriptor);

    auto typeInferencePhase = Optimizer::TypeInferencePhase::create(sourceCatalog, udfCatalog);
    auto queryPlan = typeInferencePhase->execute(query.getQueryPlan());
    queryPlan->setQueryId(0);
    queryPlan->setQuerySubPlanId(0);
    auto request = QueryCompilation::QueryCompilationRequest::create(queryPlan, nodeEngineReceiver);
    auto queryCompiler = TestUtils::createTestQueryCompiler();
    auto result = queryCompiler->compileQuery(request);
    auto builderReceiverQEP = result->getExecutableQueryPlan();

    auto networkSink = std::make_shared<TestNetworkSink>(schema,
                                                         0,
                                                         0,
                                                         1,
                                                         nodeLocationReceiver,
                                                         xPartition,
                                                         nodeEngixender,
                                                         1,
                                                         NSOURCE_RETRY_WAIT,
                                                         NSOURCE_RETRIES);

    auto testSourceDescriptor = std::make_shared<TestUtils::TestSourceDescriptor>(
        schema,
        [&](SchemaPtr schema,
            OperatorId,
            OriginId,
            const SourceDescriptorPtr&,
            const Runtime::NodeEnginePtr&,
            size_t numSourceLocalBuffers,
            std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors) -> DataSourcePtr {
            auto source = std::make_shared<TestSourceEvent>(schema,
                                                            nodeEngixender->getBufferManager(),
                                                            nodeEngixender->getQueryManager(),
                                                            1,
                                                            numSourceLocalBuffers,
                                                            std::move(successors));
            networkSink->sourceNotifier = &source->canStop;
            return source;
        });

    auto networkSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(networkSink);
    auto query2 = TestQuery::from(testSourceDescriptor).filter(Attribute("id") < 5).sink(networkSinkDescriptor);

    auto queryPlan2 = typeInferencePhase->execute(query2.getQueryPlan());
    queryPlan2->setQueryId(0);
    queryPlan2->setQuerySubPlanId(1);
    auto request2 = QueryCompilation::QueryCompilationRequest::create(queryPlan2, nodeEngixender);
    queryCompiler = TestUtils::createTestQueryCompiler();
    auto result2 = queryCompiler->compileQuery(request2);
    auto builderGeneratorQEP = result2->getExecutableQueryPlan();

    auto func = [](auto engine, auto qep) {
        return engine->registerQueryInNodeEngine(qep);
    };

    auto f1 = std::async(std::launch::async, func, nodeEngixender, builderGeneratorQEP);
    auto f2 = std::async(std::launch::async, func, nodeEngineReceiver, builderReceiverQEP);

    ASSERT_TRUE(f1.get());
    ASSERT_TRUE(f2.get());

    auto future = networkSink->completed.get_future();

    ASSERT_TRUE(nodeEngixender->startQuery(builderGeneratorQEP->getQueryId()));
    ASSERT_TRUE(nodeEngineReceiver->startQuery(builderReceiverQEP->getQueryId()));

    ASSERT_TRUE(future.get());

    ASSERT_TRUE(queryCompleted.get_future().get());

    ASSERT_TRUE(nodeEngixender->stop());
    ASSERT_TRUE(nodeEngineReceiver->stop());
}

}// namespace Network
}// namespace x