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
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/LambdaSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Common/Identifiers.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Plans/Global/Query/SharedQueryPlan.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/Execution/ExecutableQueryPlan.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Services/QueryService.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestUtils.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace std;
namespace x {

using namespace Configurations;
const int timestamp = 1644426604;
const uint64_t numberOfTupleBuffers = 4;
const uint64_t numberOfBuffersToProduceInTuples = 50000000;
const uint64_t ingestionRate = 10;

class UpstreamBackupTest : public Testing::BaseIntegrationTest {
  public:
    std::string ipAddress = "127.0.0.1";
    CoordinatorConfigurationPtr coordinatorConfig;
    WorkerConfigurationPtr workerConfig;
    LambdaSourceTypePtr lambdaSource;
    CSVSourceTypePtr csvSourceTypeFinite;
    SchemaPtr inputSchema;
    Runtime::BufferManagerPtr bufferManager;

    static void SetUpTestCase() {
        x::Logger::setupLogging("UpstreamBackupTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup UpstreamBackupTest test class.");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();

        bufferManager = std::make_shared<Runtime::BufferManager>(1024, 1);

        coordinatorConfig = CoordinatorConfiguration::createDefault();
        coordinatorConfig->rpcPort = *rpcCoordinatorPort;
        coordinatorConfig->restPort = *restPort;

        workerConfig = WorkerConfiguration::create();
        workerConfig->coordinatorPort = *rpcCoordinatorPort;

        auto func1 = [](x::Runtime::TupleBuffer& buffer, uint64_t numberOfTuplesToProduce) {
            struct Record {
                uint64_t id;
                uint64_t value;
                uint64_t timestamp;
            };

            auto* records = buffer.getBuffer<Record>();
            auto ts = time(nullptr);
            for (auto u = 0u; u < numberOfTuplesToProduce; ++u) {
                records[u].id = u;
                //values between 0..9 and the predicate is > 5 so roughly 50% selectivity
                records[u].value = u % 10;
                records[u].timestamp = ts;
            }
        };

        lambdaSource = LambdaSourceType::create(std::move(func1),
                                                numberOfBuffersToProduceInTuples,
                                                ingestionRate,
                                                GatheringMode::INGESTION_RATE_MODE);

        csvSourceTypeFinite = CSVSourceType::create();
        csvSourceTypeFinite->setFilePath(std::string(TEST_DATA_DIRECTORY) + "window-out-of-order.csv");
        csvSourceTypeFinite->setNumberOfTuplesToProducePerBuffer(numberOfTupleBuffers - 1);
        csvSourceTypeFinite->setNumberOfBuffersToProduce(numberOfTupleBuffers);

        inputSchema = Schema::create()
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());
    }
};

/*
 * @brief test timestamp of watermark processor
 */
TEST_F(UpstreamBackupTest, testTimestampWatermarkProcessor) {
    x_INFO("UpstreamBackupTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    crd->getSourceCatalogService()->registerLogicalSource("window", inputSchema);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("UpstreamBackupTest: Coordinator started successfully");

    //Setup Worker
    x_INFO("UpstreamBackupTest: Start worker 1");
    auto physicalSource1 = PhysicalSource::create("window", "x1", lambdaSource);
    workerConfig->physicalSources.add(physicalSource1);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("UpstreamBackupTest: Worker1 started successfully");

    xWorkerPtr wrk2 = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart2 = wrk2->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart2);
    x_INFO("UpstreamBackupTest: Worker2 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "testUpstreamBackup.out";
    remove(outputFilePath.c_str());

    // The query contains a watermark assignment with 50 ms allowed latexs
    x_INFO("UpstreamBackupTest: Submit query");
    string query =
        "Query::from(\"window\").sink(FileSinkDescriptor::create(\"" + outputFilePath + R"(", "CSV_FORMAT", "APPEND"));)";

    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);

    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    auto querySubPlanIds = crd->getNodeEngine()->getSubQueryIds(queryId);
    for (auto querySubPlanId : querySubPlanIds) {
        auto sinks = crd->getNodeEngine()->getExecutableQueryPlan(querySubPlanId)->getSinks();
        for (auto& sink : sinks) {
            auto buffer1 = bufferManager->getUnpooledBuffer(timestamp);
            buffer1->setWatermark(timestamp);
            buffer1->setSequenceNumber(1);
            sink->updateWatermark(buffer1.value());
            auto buffer2 = bufferManager->getUnpooledBuffer(timestamp);
            buffer2->setWatermark(timestamp);
            buffer2->setOriginId(1);
            buffer2->setSequenceNumber(1);
            sink->updateWatermark(buffer2.value());
            auto currentTimestamp = sink->getCurrentEpochBarrier();
            while (currentTimestamp == 0) {
                x_INFO("UpstreamBackupTest: current timestamp: {}", currentTimestamp);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                currentTimestamp = sink->getCurrentEpochBarrier();
            }
            EXPECT_TRUE(currentTimestamp == timestamp);
        }
    }

    x_INFO("UpstreamBackupTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("UpstreamBackupTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("UpstreamBackupTest: Stop worker 2");
    bool retStopWrk2 = wrk2->stop(true);
    EXPECT_TRUE(retStopWrk2);

    x_INFO("UpstreamBackupTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("UpstreamBackupTest: Test finished");
}
/*
 * @brief test message passing between sink-coordinator-sources
 */
TEST_F(UpstreamBackupTest, testMessagePassingSinkCoordinatorSources) {
    x_INFO("UpstreamBackupTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    crd->getSourceCatalogService()->registerLogicalSource("window", inputSchema);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("UpstreamBackupTest: Coordinator started successfully");

    //Setup Worker
    x_INFO("UpstreamBackupTest: Start worker 1");
    auto physicalSource1 = PhysicalSource::create("window", "x1", lambdaSource);
    workerConfig->physicalSources.add(physicalSource1);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("UpstreamBackupTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "testUpstreamBackup.out";
    remove(outputFilePath.c_str());

    // The query contains a watermark assignment with 50 ms allowed latexs
    x_INFO("UpstreamBackupTest: Submit query");
    string query =
        "Query::from(\"window\").sink(FileSinkDescriptor::create(\"" + outputFilePath + R"(", "CSV_FORMAT", "APPEND"));)";

    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);

    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    auto sharedQueryPlanId = globalQueryPlan->getSharedQueryId(queryId);

    //get sink
    auto querySubPlanIds = crd->getNodeEngine()->getSubQueryIds(sharedQueryPlanId);
    ASSERT_TRUE(!querySubPlanIds.empty());
    for (auto querySubPlanId : querySubPlanIds) {
        auto sinks = crd->getNodeEngine()->getExecutableQueryPlan(querySubPlanId)->getSinks();
        for (auto& sink : sinks) {
            sink->notifyEpochTermination(timestamp);
        }
    }

    auto currentTimestamp = crd->getReplicationService()->getCurrentEpochBarrier(queryId);
    while (currentTimestamp == -1) {
        x_INFO("UpstreamBackupTest: current timestamp: {}", currentTimestamp);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        currentTimestamp = crd->getReplicationService()->getCurrentEpochBarrier(queryId);
    }

    //check if the method was called
    EXPECT_TRUE(currentTimestamp == timestamp);

    x_INFO("UpstreamBackupTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("UpstreamBackupTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("UpstreamBackupTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("UpstreamBackupTest: Test finished");
}

/*
 * @brief test if upstream backup doesn't fail
 */
TEST_F(UpstreamBackupTest, testUpstreamBackupTest) {
    x_INFO("UpstreamBackupTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    crd->getSourceCatalogService()->registerLogicalSource("window", inputSchema);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("UpstreamBackupTest: Coordinator started successfully");

    //Setup Worker
    x_INFO("UpstreamBackupTest: Start worker 1");
    auto physicalSource1 = PhysicalSource::create("window", "x1", csvSourceTypeFinite);
    workerConfig->physicalSources.add(physicalSource1);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("UpstreamBackupTest: Worker1 started successfully");

    xWorkerPtr wrk2 = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart2 = wrk2->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart2);
    x_INFO("UpstreamBackupTest: Worker2 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "testUpstreamBackup.out";
    remove(outputFilePath.c_str());

    // The query contains a watermark assignment with 50 ms allowed latexs
    x_INFO("UpstreamBackupTest: Submit query");
    string query =
        "Query::from(\"window\").sink(FileSinkDescriptor::create(\"" + outputFilePath + R"(", "CSV_FORMAT", "APPEND"));)";

    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);

    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    EXPECT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("UpstreamBackupTest: Remove query");
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("UpstreamBackupTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("UpstreamBackupTest: Stop worker 2");
    bool retStopWrk2 = wrk2->stop(true);
    EXPECT_TRUE(retStopWrk2);

    x_INFO("UpstreamBackupTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("UpstreamBackupTest: Test finished");
}
}// namespace x