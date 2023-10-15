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
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Services/QueryService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>
#include <iostream>
using namespace std;

namespace x {

using namespace Configurations;

class companyDeploymentTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("companyDeploymentTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup companyDeploymentTest test class.");
    }
};

/**
 * @brief test tumbling window with multiple aggregations
 */
TEST_F(companyDeploymentTest, companyQuery) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    coordinatorConfig->worker.queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;

    //register logical source qnv
    auto companySchema = Schema::create()
                         ->addField(createField("Time", BasicType::UINT64))
                         ->addField(createField("Dist", BasicType::UINT64))
                         ->addField(createField("ABS_Front_Wheel_Press", BasicType::FLOAT64))
                         ->addField(createField("ABS_Rear_Wheel_Press", BasicType::FLOAT64))
                         ->addField(createField("ABS_Front_Wheel_Speed", BasicType::FLOAT64))
                         ->addField(createField("ABS_Rear_Wheel_Speed", BasicType::FLOAT64))
                         ->addField(createField("V_GPS", BasicType::FLOAT64))
                         ->addField(createField("MMDD", BasicType::FLOAT64))
                         ->addField(createField("HHMM", BasicType::FLOAT64))
                         ->addField(createField("LAS_Ax1", BasicType::FLOAT64))
                         ->addField(createField("LAS_Ay1", BasicType::FLOAT64))
                         ->addField(createField("LAS_Az_Vertical_Acc", BasicType::FLOAT64))
                         ->addField(createField("ABS_Lean_Angle", BasicType::FLOAT64))
                         ->addField(createField("ABS_Pitch_Info", BasicType::FLOAT64))
                         ->addField(createField("ECU_Gear_Position", BasicType::FLOAT64))
                         ->addField(createField("ECU_Accel_Position", BasicType::FLOAT64))
                         ->addField(createField("ECU_Engine_Rpm", BasicType::FLOAT64))
                         ->addField(createField("ECU_Water_Temperature", BasicType::FLOAT64))
                         ->addField(createField("ECU_Oil_Temp_Sensor_Data", BasicType::UINT64))
                         ->addField(createField("ECU_Side_StanD", BasicType::UINT64))
                         ->addField(createField("Longitude", BasicType::FLOAT64))
                         ->addField(createField("Latitude", BasicType::FLOAT64))
                         ->addField(createField("Altitude", BasicType::FLOAT64));

    x_INFO("companyDeploymentTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    crd->getSourceCatalogService()->registerLogicalSource("company", companySchema);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);//id=1
    ASSERT_EQ(port, *rpcCoordinatorPort);
    x_DEBUG("companyDeploymentTest: Coordinator started successfully");

    x_INFO("companyDeploymentTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    QueryCompilerConfiguration queryCompilerConfiguration;
    queryCompilerConfiguration.windowingStrategy = QueryCompilation::QueryCompilerOptions::WindowingStrategy::SLICING;
    workerConfig1->queryCompiler = queryCompilerConfiguration;
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;
    workerConfig1->queryCompiler.queryCompilerType =
        QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER;
    // create source
    CSVSourceTypePtr csvSourceType1 = CSVSourceType::create();
    csvSourceType1->setFilePath(std::string(TEST_DATA_DIRECTORY) + "company.csv");
    csvSourceType1->setGatheringInterval(1);
    csvSourceType1->setNumberOfTuplesToProducePerBuffer(3);
    csvSourceType1->setNumberOfBuffersToProduce(1);
    auto physicalSource1 = PhysicalSource::create("company", "test_stream", csvSourceType1);
    workerConfig1->physicalSources.add(physicalSource1);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart2 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);//id=3
    ASSERT_TRUE(retStart2);
    x_INFO("companyDeploymentTest: Worker 2 started successfully");

    std::string outputFilePath = "company-results.csv";
    remove(outputFilePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    x_INFO("companyDeploymentTest: Submit query");
    auto query = Query::from("company")
                     .window(TumblingWindow::of(EventTime(Attribute("Time")), Seconds(1)))
                     .apply(Avg(Attribute("ABS_Lean_Angle"))->as(Attribute("avg_value_1")),
                            Avg(Attribute("ABS_Pitch_Info"))->as(Attribute("avg_value_2")),
                            Avg(Attribute("ABS_Front_Wheel_Speed"))->as(Attribute("avg_value_3")),
                            Count()->as(Attribute("count_value")))
                     .sink(FileSinkDescriptor::create(outputFilePath, "CSV_FORMAT", "APPEND"));

    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    string expectedContent = "company$start:INTEGER(64 bits),company$end:INTEGER(64 bits),company$avg_value_1:Float(64 bits),company$avg_value_2:"
                             "Float(64 bits),company$avg_value_3:Float(64 bits),company$count_value:INTEGER(64 bits)\n"
                             "1543620000000,1543620001000,14.400000,0.800000,0.500000,2\n";
    EXPECT_TRUE(TestUtils::checkOutputOrTimeout(expectedContent, outputFilePath));

    x_INFO("companyDeploymentTest: Remove query");
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("companyDeploymentTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("companyDeploymentTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    remove(outputFilePath.c_str());
    x_INFO("companyDeploymentTest: Test finished");
}
}// namespace x