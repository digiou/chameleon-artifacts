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
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MaterializedViewSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/MaterializedViewSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/PrintSinkDescriptor.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace x {
class MaterializedViewTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("MaterializedViewTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup MaterializedViewTest test class.");
    }
};

/// @brief tests if a query with materialized view sink starts properly
TEST_F(MaterializedViewTest, MaterializedViewTupleViewSinkTest) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("MaterializedViewTupleViewSinkTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(false);
    EXPECT_NE(port, 0UL);
    // register logical source
    std::string source =
        R"(Schema::create()->addField(createField("value", BasicType::UINT64))->addField(createField("id", BasicType::UINT64))->addField(createField("timestamp", BasicType::UINT64));)";
    crd->getSourceCatalogService()->registerLogicalSource("stream", source);

    x_INFO("MaterializedViewTupleViewSinkTest: Coordinator started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    x_DEBUG("WindowDeploymentTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->numberOfSlots = (12);
    CSVSourceTypePtr csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath(std::string(TEST_DATA_DIRECTORY) + "window.csv");
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(1);
    csvSourceType1->setNumberOfBuffersToProduce(1000);
    csvSourceType1->setGatheringInterval(1);
    csvSourceType1->setSkipHeader(true);
    auto physicalSource1 = PhysicalSource::create("stream", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("WindowDeploymentTest: Worker1 started successfully");

    Query query = Query::from("stream").sink(x::Experimental::MaterializedView::MaterializedViewSinkDescriptor::create(1));
    auto queryPlan = query.getQueryPlan();
    queryPlan->setQueryId(1);
    auto queryId = queryService->addQueryRequest("", queryPlan, Optimizer::PlacementStrategy::BottomUp);

    x_INFO("MaterializedViewTupleViewSinkTest: queryId {}", queryId);
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    x_INFO("MaterializedViewTupleViewSinkTest: Remove query");
    // queryService->validateAndQueueStopQueryRequest(queryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("MaterializedViewTupleViewSinkTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("MaterializedViewTupleViewSinkTest: Test finished");
}

/// @brief tests if a query with materialized view source starts properly
TEST_F(MaterializedViewTest, MaterializedViewTupleBufferSourceTest) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("MaterializedViewTupleBufferSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(false);
    EXPECT_NE(port, 0UL);
    //register logical source
    std::string source =
        R"(Schema::create()->addField(createField("value", BasicType::UINT64))->addField(createField("id", BasicType::UINT64))->addField(createField("timestamp", BasicType::UINT64));)";
    crd->getSourceCatalogService()->registerLogicalSource("stream", source);
    x_INFO("MaterializedViewTupleBufferSourceTest: Coordinator started successfully");

    x_DEBUG("WindowDeploymentTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->coordinatorPort = port;
    workerConfig1->numberOfSlots = (12);
    // materialized view physical source
    size_t viewId = 1;
    auto materializeViewSourceType = Configurations::Experimental::MaterializedView::MaterializedViewSourceType::create();
    materializeViewSourceType->setId(viewId);
    auto physicalSource1 = PhysicalSource::create("stream", "MV", materializeViewSourceType);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("WindowDeploymentTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    Query adhoc_query = Query::from("stream").sink(PrintSinkDescriptor::create());
    auto adhoc_queryPlan = adhoc_query.getQueryPlan();
    adhoc_queryPlan->setQueryId(1);
    auto adhocQueryId = queryService->addQueryRequest("", adhoc_queryPlan, Optimizer::PlacementStrategy::BottomUp);

    x_INFO("MaterializedViewTupleBufferSourceTest: queryId {}", adhocQueryId);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(adhocQueryId, queryCatalogService));

    x_INFO("MaterializedViewTupleBufferSourceTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(adhocQueryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(adhocQueryId, queryCatalogService));

    x_INFO("MaterializedViewTupleBufferSourceTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("MaterializedViewTupleBufferSourceTest: Test finished");
}

// @brief tests with two concurrent queryIdAndCatalogEntryMapping if writing and reading of MVs works properly
TEST_F(MaterializedViewTest, MaterializedViewTupleBufferSinkAndSourceTest) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(false);
    EXPECT_NE(port, 0UL);
    std::string source =
        R"(Schema::create()->addField(createField("value", BasicType::UINT64))->addField(createField("id", BasicType::UINT64))->addField(createField("timestamp", BasicType::UINT64));)";
    crd->getSourceCatalogService()->registerLogicalSource("stream", source);
    crd->getSourceCatalogService()->registerLogicalSource("stream2", source);
    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: Coordinator started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    x_DEBUG("WindowDeploymentTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = port;
    workerConfig1->numberOfSlots = (12);
    // materialized view physical source
    size_t viewId = 1;
    auto materializeViewSourceType = Configurations::Experimental::MaterializedView::MaterializedViewSourceType::create();
    auto physicalSource1 = PhysicalSource::create("stream2", "MV", materializeViewSourceType);
    materializeViewSourceType->setId(viewId);
    CSVSourceTypePtr csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath(std::string(TEST_DATA_DIRECTORY) + "window.csv");
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(1);
    csvSourceType1->setNumberOfBuffersToProduce(1000);
    csvSourceType1->setGatheringInterval(1);
    csvSourceType1->setSkipHeader(true);
    auto physicalSource2 = PhysicalSource::create("stream", "test_stream", materializeViewSourceType);
    workerConfig1->physicalSources.add(physicalSource1);
    workerConfig1->physicalSources.add(physicalSource2);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("WindowDeploymentTest: Worker1 started successfully");

    Query maintenance_query =
        Query::from("stream").sink(x::Experimental::MaterializedView::MaterializedViewSinkDescriptor::create(viewId));
    auto maintenance_queryPlan = maintenance_query.getQueryPlan();
    maintenance_queryPlan->setQueryId(1);
    auto maintenanceQueryId = queryService->addQueryRequest("", maintenance_queryPlan, Optimizer::PlacementStrategy::BottomUp);

    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: queryId {}", maintenanceQueryId);
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(maintenanceQueryId, queryCatalogService));

    Query adhoc_query = Query::from("stream2").sink(PrintSinkDescriptor::create());
    auto adhoc_queryPlan = adhoc_query.getQueryPlan();
    adhoc_queryPlan->setQueryId(2);
    auto adhocQueryId = queryService->addQueryRequest("", adhoc_queryPlan, Optimizer::PlacementStrategy::BottomUp);

    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: queryId {}", adhocQueryId);
    EXPECT_TRUE(TestUtils::waitForQueryToStart(adhocQueryId, queryCatalogService));

    // check if queries stopped
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(maintenanceQueryId, queryCatalogService));
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(adhocQueryId, queryCatalogService));

    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("MaterializedViewTupleBufferSinkAndSourceTest: Test finished");
}
}// namespace x