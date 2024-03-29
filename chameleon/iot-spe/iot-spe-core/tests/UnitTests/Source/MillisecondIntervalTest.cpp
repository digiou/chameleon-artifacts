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

#include <BaseIntegrationTest.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/DefaultSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/NodeEngineBuilder.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Services/QueryService.hpp>
#include <Sinks/Mediums/FileSink.hpp>
#include <Sinks/SinkCreator.hpp>
#include <Sources/SourceCreator.hpp>
#include <Util/TestUtils.hpp>

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace x::Runtime;
using namespace x::Runtime::Execution;

namespace x {

using namespace Configurations;

struct __attribute__((packed)) ysbRecord {
    char user_id[16]{};
    char page_id[16]{};
    char campaign_id[16]{};
    char ad_type[9]{};
    char event_type[9]{};
    int64_t current_ms;
    uint32_t ip;

    ysbRecord() {
        event_type[0] = '-';// invalid record
        event_type[1] = '\0';
        current_ms = 0;
        ip = 0;
    }

    ysbRecord(const ysbRecord& rhs) {
        memcpy(&user_id, &rhs.user_id, 16);
        memcpy(&page_id, &rhs.page_id, 16);
        memcpy(&campaign_id, &rhs.campaign_id, 16);
        memcpy(&ad_type, &rhs.ad_type, 9);
        memcpy(&event_type, &rhs.event_type, 9);
        current_ms = rhs.current_ms;
        ip = rhs.ip;
    }
};
// size 78 bytes

/**
 * This test set holds the corner cases for moving our sampling frequencies to
 * sub-second intervals. Before, x was sampling every second and was checking
 * every second if that future timestamp is now stale (older).
 *
 * First we check for sub-second unit-tests on a soruce and its behavior. Then,
 * we include an E2Etest with a source that samples at sub-second interval.
 */
class MillisecondIntervalTest : public Testing::BaseIntegrationTest {
  public:
    CoordinatorConfigurationPtr coordinatorConfig;
    WorkerConfigurationPtr wrkConf;
    CSVSourceTypePtr csvSourceType;
    std::string defaultPhysicalStreamName = "defaultPhysicalStreamName";

    static void SetUpTestCase() {
        x::Logger::setupLogging("MillisecondIntervalTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup MillisecondIntervalTest test class.");
    }

    void SetUp() override {

        Testing::BaseIntegrationTest::SetUp();

        csvSourceType = CSVSourceType::create();
        csvSourceType->setGatheringInterval(550);
        csvSourceType->setNumberOfTuplesToProducePerBuffer(1);
        csvSourceType->setNumberOfBuffersToProduce(3);
        PhysicalSourcePtr sourceConf = PhysicalSource::create("testStream", "physical_test", csvSourceType);
        auto workerConfigurations = WorkerConfiguration::create();
        workerConfigurations->physicalSources.add(sourceConf);
        this->nodeEngine = Runtime::NodeEngineBuilder::create(workerConfigurations)
                               .setQueryStatusListener(std::make_shared<DummyQueryListener>())
                               .build();

        coordinatorConfig = CoordinatorConfiguration::createDefault();
        coordinatorConfig->rpcPort = *rpcCoordinatorPort;
        coordinatorConfig->restPort = *restPort;

        wrkConf = WorkerConfiguration::create();
        wrkConf->coordinatorPort = *rpcCoordinatorPort;

        path_to_file = std::string(TEST_DATA_DIRECTORY) + "ysb-tuples-100-campaign-100.csv";

        x_INFO("Setup MillisecondIntervalTest class.");
    }

    void TearDown() override {
        ASSERT_TRUE(nodeEngine->stop());
        x_INFO("Tear down MillisecondIntervalTest test case.");
        Testing::BaseIntegrationTest::TearDown();
    }

    Runtime::NodeEnginePtr nodeEngine{nullptr};
    std::string path_to_file;
};// MillisecondIntervalTest

class MockedPipelineExecutionContext : public Runtime::Execution::PipelineExecutionContext {
  public:
    MockedPipelineExecutionContext(Runtime::QueryManagerPtr queryManager, DataSinkPtr sink)
        : PipelineExecutionContext(
            -1,// mock pipeline id
            0, // mock query id
            queryManager->getBufferManager(),
            queryManager->getNumberOfWorkerThreads(),
            [sink](TupleBuffer& buffer, Runtime::WorkerContextRef worker) {
                sink->writeData(buffer, worker);
            },
            [sink](TupleBuffer&) {
            },
            std::vector<Runtime::Execution::OperatorHandlerPtr>()){
            // nop
        };
};

class MockedExecutablePipeline : public ExecutablePipelixtage {
  public:
    std::atomic<uint64_t> count = 0;
    std::promise<bool> completedPromise;

    ExecutionResult
    execute(TupleBuffer& inputTupleBuffer, PipelineExecutionContext& pipelineExecutionContext, WorkerContext& wctx) override {
        count += inputTupleBuffer.getNumberOfTuples();

        TupleBuffer outputBuffer = wctx.allocateTupleBuffer();
        auto arr = outputBuffer.getBuffer<uint32_t>();
        arr[0] = static_cast<uint32_t>(count.load());
        outputBuffer.setNumberOfTuples(count);
        pipelineExecutionContext.emitBuffer(outputBuffer, wctx);
        completedPromise.set_value(true);
        return ExecutionResult::Ok;
    }
};

TEST_F(MillisecondIntervalTest, testPipelinedCSVSource) {
    // Related to https://github.com/IoTSPE/IoTSPE/issues/2035
    auto queryId = 1;
    double frequency = 550;
    SchemaPtr schema = Schema::create()
                           ->addField("user_id", DataTypeFactory::createFixedChar(16))
                           ->addField("page_id", DataTypeFactory::createFixedChar(16))
                           ->addField("campaign_id", DataTypeFactory::createFixedChar(16))
                           ->addField("ad_type", DataTypeFactory::createFixedChar(9))
                           ->addField("event_type", DataTypeFactory::createFixedChar(9))
                           ->addField("current_ms", BasicType::UINT64)
                           ->addField("ip", BasicType::INT32);
    uint64_t tuple_size = schema->getSchemaSizeInBytes();
    uint64_t buffer_size = nodeEngine->getBufferManager()->getBufferSize();
    uint64_t numberOfBuffers = 1;
    uint64_t numberOfTuplesToProcess = numberOfBuffers * (buffer_size / tuple_size);

    auto sink = createCSVFileSink(schema, queryId, queryId, this->nodeEngine, 1, "qep1.txt", false);
    auto context = std::make_shared<MockedPipelineExecutionContext>(this->nodeEngine->getQueryManager(), sink);
    auto executableStage = std::make_shared<MockedExecutablePipeline>();
    auto pipeline =
        ExecutablePipeline::create(0, queryId, queryId, this->nodeEngine->getQueryManager(), context, executableStage, 1, {sink});

    CSVSourceTypePtr csvSourceType = CSVSourceType::create();
    csvSourceType->setFilePath(this->path_to_file);
    csvSourceType->setNumberOfBuffersToProduce(numberOfBuffers);
    csvSourceType->setNumberOfTuplesToProducePerBuffer(numberOfTuplesToProcess);
    csvSourceType->setGatheringInterval(frequency);

    auto source = createCSVFileSource(schema,
                                      this->nodeEngine->getBufferManager(),
                                      this->nodeEngine->getQueryManager(),
                                      csvSourceType,
                                      1,
                                      0,
                                      12,
                                      defaultPhysicalStreamName,
                                      {pipeline});

    auto executionPlan = ExecutableQueryPlan::create(queryId,
                                                     queryId,
                                                     {source},
                                                     {sink},
                                                     {pipeline},
                                                     this->nodeEngine->getQueryManager(),
                                                     this->nodeEngine->getBufferManager());
    EXPECT_TRUE(this->nodeEngine->registerQueryInNodeEngine(executionPlan));
    EXPECT_TRUE(this->nodeEngine->startQuery(1));
    EXPECT_EQ(this->nodeEngine->getQueryStatus(1), ExecutableQueryPlanStatus::Running);
    executableStage->completedPromise.get_future().get();
}

TEST_F(MillisecondIntervalTest, DISABLED_testCSVSourceWithOneLoopOverFileSubSecond) {
    auto nodeEngine = this->nodeEngine;

    double frequency = 550;
    SchemaPtr schema = Schema::create()
                           ->addField("user_id", DataTypeFactory::createFixedChar(16))
                           ->addField("page_id", DataTypeFactory::createFixedChar(16))
                           ->addField("campaign_id", DataTypeFactory::createFixedChar(16))
                           ->addField("ad_type", DataTypeFactory::createFixedChar(9))
                           ->addField("event_type", DataTypeFactory::createFixedChar(9))
                           ->addField("current_ms", BasicType::UINT64)
                           ->addField("ip", BasicType::INT32);

    uint64_t tuple_size = schema->getSchemaSizeInBytes();
    uint64_t buffer_size = nodeEngine->getBufferManager()->getBufferSize();
    uint64_t numberOfBuffers = 1;
    uint64_t numberOfTuplesToProcess = numberOfBuffers * (buffer_size / tuple_size);

    CSVSourceTypePtr csvSourceType = CSVSourceType::create();
    csvSourceType->setFilePath(this->path_to_file);
    csvSourceType->setNumberOfBuffersToProduce(numberOfBuffers);
    csvSourceType->setNumberOfTuplesToProducePerBuffer(numberOfTuplesToProcess);
    csvSourceType->setGatheringInterval(frequency);

    const DataSourcePtr source = createCSVFileSource(schema,
                                                     nodeEngine->getBufferManager(),
                                                     nodeEngine->getQueryManager(),
                                                     csvSourceType,
                                                     1,
                                                     0,
                                                     12,
                                                     defaultPhysicalStreamName,
                                                     {});
    source->start();
    while (source->getNumberOfGeneratedBuffers() < numberOfBuffers) {
        auto optBuf = source->receiveData();
        // will be handled by issue #1612, test is disabled
        // use WindowDeploymentTest->testYSBWindow, where getBuffer is cast to ysbRecord
        uint64_t i = 0;
        while (i * tuple_size < buffer_size - tuple_size && optBuf.has_value()) {
            auto* record = optBuf->getBuffer<ysbRecord>() + i;
            std::cout << "i=" << i << " record.ad_type: " << record->ad_type << ", record.event_type: " << record->event_type
                      << std::endl;
            EXPECT_STREQ(record->ad_type, "banner78");
            EXPECT_TRUE((!strcmp(record->event_type, "view") || !strcmp(record->event_type, "click")
                         || !strcmp(record->event_type, "purchase")));
            i++;
        }
    }

    EXPECT_EQ(source->getNumberOfGeneratedTuples(), numberOfTuplesToProcess);
    EXPECT_EQ(source->getNumberOfGeneratedBuffers(), numberOfBuffers);
    EXPECT_EQ(source->getGatheringIntervalCount(), frequency);
}

TEST_F(MillisecondIntervalTest, testMultipleOutputBufferFromDefaultSourcePrintSubSecond) {
    x_INFO("MillisecondIntervalTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0u);
    //register logical source
    std::string testSchema = "Schema::create()->addField(createField(\"campaign_id\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_INFO("MillisecondIntervalTest: Coordinator started successfully");

    x_INFO("MillisecondIntervalTest: Start worker 1");
    wrkConf->coordinatorPort = port;
    auto defaultSourceType = DefaultSourceType::create();
    defaultSourceType->setNumberOfBuffersToProduce(3);
    auto physicalSource = PhysicalSource::create("testStream", "x1", defaultSourceType);
    wrkConf->physicalSources.add(physicalSource);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(wrkConf));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("MillisecondIntervalTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    std::string queryString =
        R"(Query::from("testStream").filter(Attribute("campaign_id") < 42).sink(PrintSinkDescriptor::create());)";

    QueryId queryId = queryService->validateAndQueueAddQueryRequest(queryString,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 3));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 3));

    x_INFO("MillisecondIntervalTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    bool retStopWrk = wrk1->stop(false);
    EXPECT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    EXPECT_TRUE(retStopCord);
}

}//namespace x