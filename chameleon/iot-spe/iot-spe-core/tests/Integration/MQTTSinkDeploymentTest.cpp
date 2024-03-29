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
#include <gtest/gtest.h>

#include <Common/Identifiers.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Services/QueryService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestUtils.hpp>

using namespace std;

namespace x {

class MQTTSinkDeploymentTest : public Testing::BaseIntegrationTest {
  public:
    CoordinatorConfigurationPtr coConf;
    WorkerConfigurationPtr wrkConf;

    static void SetUpTestCase() {
        x::Logger::setupLogging("MQTTSinkDeploymentTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup MQTTSinkDeploymentTest test class.");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();
        coConf = CoordinatorConfiguration::createDefault();
        wrkConf = WorkerConfiguration::create();
        coConf->rpcPort = (*rpcCoordinatorPort);
        coConf->restPort = *restPort;
        wrkConf->coordinatorPort = *rpcCoordinatorPort;
    }

    void TearDown() override {
        x_INFO("Tear down MQTTSinkDeploymentTest class.");
        Testing::BaseIntegrationTest::TearDown();
    }
};

/**
 * Test deploying an MQTT sink and sending data via the deployed sink to an MQTT broker
 * DISABLED for now, because it requires a manually set up MQTT broker -> fails otherwise
 */

TEST_F(MQTTSinkDeploymentTest, DISABLED_testDeployOneWorker) {
    x_INFO("MQTTSinkDeploymentTest: Start coordinator");
    // Here the default schema (default_logical) is already initialized (xCoordinator calls 'SourceCatalog'
    // it is later used in TypeInferencePhase.cpp via 'sourceCatalog->getSchemaForLogicalSource(logicalSourceName);' to set
    // the new sources schema to the default_logical schema
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coConf);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("MQTTSinkDeploymentTest: Coordinator started successfully");

    x_INFO("MQTTSinkDeploymentTest: Start worker 1");
    wrkConf->coordinatorPort = port;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(wrkConf));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("MQTTSinkDeploymentTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "testDeployOneWorker.out";
    remove(outputFilePath.c_str());

    x_INFO("MQTTSinkDeploymentTest: Submit query");

    // arguments are given so that ThingsBoard accepts the messages sent by the MQTT client
    string query = R"(Query::from("default_logical").sink(MQTTSinkDescriptor::create("ws://127.0.0.1:9001",
            "/xui", "rfRqLGZRChg8eS30PEeR", 5, MQTTSinkDescriptor::milliseconds, 500, MQTTSinkDescriptor::atLeastOnce, false));)";

    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);

    // Comment for better understanding: From here on at some point the DataSource.cpp 'runningRoutine()' function is called
    // this function, because "default_logical" is used, uses 'DefaultSource.cpp', which create a TupleBuffer with 10 id:value
    // pairs, each being 1,1
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 1));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 1));

    x_INFO("MQTTSinkDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("MQTTSinkDeploymentTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("MQTTSinkDeploymentTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("MQTTSinkDeploymentTest: Test finished");
}

}// namespace x
