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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy-dtor"
#include <BaseIntegrationTest.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#pragma clang diagnostic pop

#include <API/QueryAPI.hpp>
#include <Catalogs/Query/QueryCatalog.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/DefaultSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MemorySourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MQTTSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Services/QueryService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>
#include <iostream>

using namespace std;

#define DEBUG_OUTPUT
namespace x {

using namespace Configurations;

//FIXME: This is a hack to fix issue with unreleased RPC port after shutting down the servers while running tests in continuous succession
// by assigning a different RPC port for each test case

class ContinuousSourceTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("ContinuousSourceTest.log", x::LogLevel::LOG_NONE);
        x_INFO("Setup ContinuousSourceTest test class.");
    }

  int latencyBenchBuffers{10000};
};

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromDefaultSourcePrint) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    auto testSchema = Schema::create()->addField(createField("campaign_id", BasicType::UINT64));
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    auto defaultSourceType1 = DefaultSourceType::create();
    defaultSourceType1->setNumberOfBuffersToProduce(3);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", defaultSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("campaign_id") < 42).sink(PrintSinkDescriptor::create());

    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 3));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 3));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromDefaultSourcePrintWithLargerFrequency) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    auto testSchema = Schema::create()->addField(createField("campaign_id", BasicType::UINT64));
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    auto defaultSourceType1 = DefaultSourceType::create();
    defaultSourceType1->setSourceGatheringInterval(3);
    defaultSourceType1->setNumberOfBuffersToProduce(3);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", defaultSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("campaign_id") < 42).sink(PrintSinkDescriptor::create());

    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 3));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 3));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromDefaultSourceWriteFile) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    auto testSchema = Schema::create()->addField(createField("campaign_id", BasicType::UINT64));
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    auto defaultSourceType1 = DefaultSourceType::create();
    defaultSourceType1->setSourceGatheringInterval(1);
    defaultSourceType1->setNumberOfBuffersToProduce(3);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", defaultSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    std::string outputFilePath = getTestResourceFolder() / "testMultipleOutputBufferFromDefaultSourceWriteFile.txt";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("campaign_id") < 42).sink(FileSinkDescriptor::create(outputFilePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 3));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 3));

    x_INFO("QueryDeploymentTest: Remove query");
    //ASSERT_TRUE(queryService->validateAndQueueStopQueryRequest(queryId));
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "testStream$campaign_id:INTEGER(64 bits)\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n";

    cout << "content=" << content << endl;
    cout << "expContent=" << expectedContent << endl;

    std::string testOut = "expect.txt";
    std::ofstream outT(testOut);
    outT << expectedContent;
    outT.close();

    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromDefaultSourceWriteFileWithLargerFrequency) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    auto testSchema = Schema::create()->addField(createField("campaign_id", BasicType::UINT64));
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    auto defaultSourceType1 = DefaultSourceType::create();
    defaultSourceType1->setSourceGatheringInterval(1);
    defaultSourceType1->setNumberOfBuffersToProduce(3);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", defaultSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    std::string outputFilePath =
        getTestResourceFolder() / "testMultipleOutputBufferFromDefaultSourceWriteFileWithLargerFrequency.txt";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("campaign_id") < 42).sink(FileSinkDescriptor::create(outputFilePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 3));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 3));

    x_INFO("QueryDeploymentTest: Remove query");
    //ASSERT_TRUE(queryService->validateAndQueueStopQueryRequest(queryId));
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "testStream$campaign_id:INTEGER(64 bits)\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n"
                             "1\n";

    cout << "content=" << content << endl;
    cout << "expContent=" << expectedContent << endl;

    std::string testOut = "expect.txt";
    std::ofstream outT(testOut);
    outT << expectedContent;
    outT.close();

    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromCSVSourcePrint) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    auto testSchema = Schema::create()
                          ->addField(createField("val1", BasicType::UINT64))
                          ->addField(createField("val2", BasicType::UINT64))
                          ->addField(createField("val3", BasicType::UINT64));
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    std::string testCSV = "1,2,3\n"
                          "1,2,4\n"
                          "4,3,6";
    std::string testCSVFileName = "testCSV.csv";
    std::ofstream outCsv(testCSVFileName);
    outCsv << testCSV;
    outCsv.close();
    auto csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath("testCSV.csv");
    csvSourceType1->setGatheringInterval(0);
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(0);
    csvSourceType1->setNumberOfBuffersToProduce(3);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("val1") < 2).sink(PrintSinkDescriptor::create());
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    //ASSERT_TRUE(queryService->validateAndQueueStopQueryRequest(queryId));
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMultipleOutputBufferFromCSVSourceWrite) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"val1\", BasicType::UINT64))->"
                             "addField(createField(\"val2\", BasicType::UINT64))->"
                             "addField(createField(\"val3\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    std::string testCSV = "1,2,3\n"
                          "1,2,4\n"
                          "4,3,6";
    std::string testCSVFileName = "testCSV.csv";
    std::ofstream outCsv(testCSVFileName);
    outCsv << testCSV;
    outCsv.close();
    auto csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath("testCSV.csv");
    csvSourceType1->setGatheringInterval(0);
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(0);
    csvSourceType1->setNumberOfBuffersToProduce(1);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    std::string outputFilePath = getTestResourceFolder() / "testMultipleOutputBufferFromCSVSourceWriteTest.out";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("val1") < 10).sink(FileSinkDescriptor::create(outputFilePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    //ASSERT_TRUE(queryService->validateAndQueueStopQueryRequest(queryId));
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent =
        "testStream$val1:INTEGER(64 bits),testStream$val2:INTEGER(64 bits),testStream$val3:INTEGER(64 bits)\n"
        "1,2,3\n"
        "1,2,4\n"
        "4,3,6\n";
    x_INFO("ContinuousSourceTest: content={}", content);
    x_INFO("ContinuousSourceTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testTimestampChameleonSource) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"id\", BasicType::UINT64))->"
                             "addField(createField(\"value\", BasicType::FLOAT64))->"
                             "addField(createField(\"payload\", BasicType::UINT64))->"
                             "addField(createField(\"timestamp\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    std::string testCSV = "1,2,3,1695902043651\n"
                          "1,2,4,1695902043651\n"
                          "4,3,6,1695902043651";
    std::string testCSVFileName = "testCSV.csv";
    std::ofstream outCsv(testCSVFileName);
    outCsv << testCSV;
    outCsv.close();
    auto csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath("testCSV.csv");
    csvSourceType1->setGatheringInterval(4000);
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(0);
    csvSourceType1->setNumberOfBuffersToProduce(1);
    csvSourceType1->setGatheringMode(x::GatheringMode::ADAPTIVE_MODE);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    std::string outputFilePath = getTestResourceFolder() / "testTimestampCsvSink.out";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("id") < 10).sink(FileSinkDescriptor::create(outputFilePath, true));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    x_INFO("ContinuousSourceTest: content=\n{}", content);
    EXPECT_EQ(countOccurrences("\n", content), 4);
    EXPECT_EQ(countOccurrences(",", content), 4 * 4);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMQTTLatencyChameleon) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"id\", BasicType::UINT64))->"
                             "addField(createField(\"value\", BasicType::FLOAT64))->"
                             "addField(createField(\"payload\", BasicType::UINT64))->"
                             "addField(createField(\"evt_timestamp\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;

    auto mqttSourceType1 = MQTTSourceType::create();
    mqttSourceType1->setGatheringInterval(4000);
    mqttSourceType1->setGatheringMode(x::GatheringMode::ADAPTIVE_MODE);
    mqttSourceType1->setNumberOfBuffersToProduce(this->latencyBenchBuffers);
    mqttSourceType1->setNumberOfTuplesToProducePerBuffer(1);
    mqttSourceType1->setUrl("localhost:1883");
    mqttSourceType1->setClientId("test-client");
    mqttSourceType1->setUserName("testUser");
    mqttSourceType1->setTopic("benchmark");
    mqttSourceType1->setQos(0);
    mqttSourceType1->setCleanSession(true);
    mqttSourceType1->setFlushIntervalMS(0);
    mqttSourceType1->setInputFormat(x::InputFormat::CSV);
    auto physicalMQTTSource = PhysicalSource::create("testStream", "test_stream", mqttSourceType1);
    workerConfig1->physicalSources.add(physicalMQTTSource);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << now_c << "-" << "testChameleonMQTTLatency-adaptive.csv";

    std::string outputFilePath = ss.str();
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    auto evtTimeQuery = Query::from("testStream")
            .window(TumblingWindow::of(EventTime(Attribute("evt_timestamp")), Seconds(1)))
            .byKey(Attribute("id"))
            .apply(Max(Attribute("evt_timestamp")))
            .sink(FileSinkDescriptor::create(outputFilePath, true));

    QueryId queryId = queryService->addQueryRequest(evtTimeQuery.getQueryPlan()->toString(),
                                                    evtTimeQuery.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService, std::chrono::seconds(600)));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMQTTLatencyChameleonOversampler) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"id\", BasicType::UINT64))->"
                             "addField(createField(\"value\", BasicType::FLOAT64))->"
                             "addField(createField(\"payload\", BasicType::UINT64))->"
                             "addField(createField(\"evt_timestamp\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;

    auto mqttSourceType1 = MQTTSourceType::create();
    mqttSourceType1->setGatheringInterval(4000);
    mqttSourceType1->setGatheringMode(x::GatheringMode::ADAPTIVE_MODE_OVERSAMPLER);
    mqttSourceType1->setNumberOfBuffersToProduce(this->latencyBenchBuffers);
    mqttSourceType1->setNumberOfTuplesToProducePerBuffer(1);
    mqttSourceType1->setUrl("localhost:1883");
    mqttSourceType1->setClientId("test-client");
    mqttSourceType1->setUserName("testUser");
    mqttSourceType1->setTopic("benchmark");
    mqttSourceType1->setQos(0);
    mqttSourceType1->setCleanSession(true);
    mqttSourceType1->setFlushIntervalMS(0);
    mqttSourceType1->setInputFormat(x::InputFormat::CSV);
    auto physicalMQTTSource = PhysicalSource::create("testStream", "test_stream", mqttSourceType1);
    workerConfig1->physicalSources.add(physicalMQTTSource);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << now_c << "-" << "testChameleonMQTTLatency-adaptive-o.csv";

    std::string outputFilePath = ss.str();
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    auto evtTimeQuery = Query::from("testStream")
            .window(TumblingWindow::of(EventTime(Attribute("evt_timestamp")), Seconds(1)))
            .byKey(Attribute("id"))
            .apply(Max(Attribute("evt_timestamp")))
            .sink(FileSinkDescriptor::create(outputFilePath, true));

    QueryId queryId = queryService->addQueryRequest(evtTimeQuery.getQueryPlan()->toString(),
                                                    evtTimeQuery.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService, std::chrono::seconds(600)));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testMQTTLatencyIntervalSource) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"id\", BasicType::UINT64))->"
                             "addField(createField(\"value\", BasicType::FLOAT64))->"
                             "addField(createField(\"payload\", BasicType::UINT64))->"
                             "addField(createField(\"evt_timestamp\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;

    auto mqttSourceType1 = MQTTSourceType::create();
    mqttSourceType1->setGatheringInterval(4000);
    mqttSourceType1->setGatheringMode(x::GatheringMode::INTERVAL_MODE);
    mqttSourceType1->setNumberOfBuffersToProduce(this->latencyBenchBuffers);
    mqttSourceType1->setNumberOfTuplesToProducePerBuffer(1);
    mqttSourceType1->setUrl("localhost:1883");
    mqttSourceType1->setClientId("test-client");
    mqttSourceType1->setUserName("testUser");
    mqttSourceType1->setTopic("benchmark");
    mqttSourceType1->setQos(0);
    mqttSourceType1->setCleanSession(true);
    mqttSourceType1->setFlushIntervalMS(0);
    mqttSourceType1->setInputFormat(x::InputFormat::CSV);
    auto physicalMQTTSource = PhysicalSource::create("testStream", "test_stream", mqttSourceType1);
    workerConfig1->physicalSources.add(physicalMQTTSource);

    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << now_c << "-" << "testChameleonMQTTLatency-fixed.csv";

    std::string outputFilePath = ss.str();
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    auto evtTimeQuery = Query::from("testStream")
                            .window(TumblingWindow::of(EventTime(Attribute("evt_timestamp")), Seconds(1)))
                            .byKey(Attribute("id"))
                            .apply(Max(Attribute("evt_timestamp")))
                            .sink(FileSinkDescriptor::create(outputFilePath, true));

    QueryId queryId = queryService->addQueryRequest(evtTimeQuery.getQueryPlan()->toString(),
                                                    evtTimeQuery.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService, std::chrono::seconds(600)));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(ContinuousSourceTest, testTimestampCsvSink) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("ContinuousSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    EXPECT_NE(port, 0UL);
    std::string testSchema = "Schema::create()->addField(createField(\"val1\", BasicType::UINT64))->"
                             "addField(createField(\"val2\", BasicType::UINT64))->"
                             "addField(createField(\"val3\", BasicType::UINT64));";
    crd->getSourceCatalogService()->registerLogicalSource("testStream", testSchema);
    x_DEBUG("ContinuousSourceTest: Coordinator started successfully");

    x_DEBUG("ContinuousSourceTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    std::string testCSV = "1,2,3\n"
                          "1,2,4\n"
                          "4,3,6";
    std::string testCSVFileName = "testCSV.csv";
    std::ofstream outCsv(testCSVFileName);
    outCsv << testCSV;
    outCsv.close();
    auto csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath("testCSV.csv");
    csvSourceType1->setGatheringInterval(0);
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(0);
    csvSourceType1->setNumberOfBuffersToProduce(1);
    auto physicalSource1 = PhysicalSource::create("testStream", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("ContinuousSourceTest: Worker1 started successfully");

    std::string outputFilePath = getTestResourceFolder() / "testTimestampCsvSink.out";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("testStream").filter(Attribute("val1") < 10).sink(FileSinkDescriptor::create(outputFilePath, true));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("QueryDeploymentTest: Remove query");
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    std::ifstream ifs(outputFilePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    x_INFO("ContinuousSourceTest: content=\n{}", content);
    EXPECT_EQ(countOccurrences("\n", content), 4);
    EXPECT_EQ(countOccurrences(",", content), 3 * 4);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/*
 * Testing test harxs CSV source
 */
TEST_F(ContinuousSourceTest, testWithManyInputBuffer) {
    uint64_t numBufferToProduce = 1000;

    struct Car {
        uint32_t key;
        uint32_t value;
        uint64_t timestamp;
    };

    auto carSchema = Schema::create()
                         ->addField("key", DataTypeFactory::createUInt32())
                         ->addField("value", DataTypeFactory::createUInt32())
                         ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(Car), carSchema->getSchemaSizeInBytes());

    auto csvSourceType = CSVSourceType::create();
    csvSourceType->setFilePath(std::string(TEST_DATA_DIRECTORY) + "long_running.csv");
    csvSourceType->setGatheringInterval(0);
    csvSourceType->setNumberOfTuplesToProducePerBuffer(1);
    csvSourceType->setNumberOfBuffersToProduce(numBufferToProduce);
    csvSourceType->setSkipHeader(false);

    auto queryWithFilterOperator = Query::from("car");
    TestHarxs testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                                  .enableNautilus()
                                  .addLogicalSource("car", carSchema)
                                  .attachWorkerWithCSVSourceToCoordinator("car", csvSourceType)
                                  .validate()
                                  .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1ULL);

    struct Output {
        uint32_t key;
        uint32_t value;
        uint64_t timestamp;

        bool operator==(Output const& rhs) const { return (key == rhs.key && value == rhs.value && timestamp == rhs.timestamp); }
    };
    std::vector<Output> expectedOutput = {};
    for (uint64_t i = 0; i < numBufferToProduce; i++) {
        expectedOutput.push_back({1, 1, (i + 1) * 100});
    }
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size());

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

}// namespace x
