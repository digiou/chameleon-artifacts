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

#ifdef ENABLE_MQTT_BUILD
#include <API/Schema.hpp>
#include <BaseIntegrationTest.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MQTTSourceType.hpp>
#include <Operators/LogicalOperators/Sources/MQTTSourceDescriptor.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/NodeEngineBuilder.hpp>
#include <Sources/SourceCreator.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include <Common/Identifiers.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Util/TestUtils.hpp>

#ifndef OPERATORID
#define OPERATORID 1
#endif

#ifndef ORIGINID
#define ORIGINID 1
#endif

#ifndef NUMSOURCELOCALBUFFERS
#define NUMSOURCELOCALBUFFERS 12
#endif

#ifndef PHYSICALSOURCENAME
#define PHYSICALSOURCENAME "defaultPhysicalSourceName"
#endif

#ifndef SUCCESSORS
#define SUCCESSORS                                                                                                               \
    {}
#endif

#ifndef INPUTFORMAT
#define INPUTFORMAT SourceDescriptor::InputFormat::JSON
#endif

namespace x {

class MQTTSourceTest : public Testing::BaseIntegrationTest {
  public:
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("MQTTSourceTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("MQTTSOURCETEST::SetUpTestCase()");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();
        x_DEBUG("MQTTSOURCETEST::SetUp() MQTTSourceTest cases set up.");
        test_schema = Schema::create()->addField("var", BasicType::UINT32);
        mqttSourceType = MQTTSourceType::create();
        auto workerConfigurations = WorkerConfiguration::create();
        nodeEngine = Runtime::NodeEngineBuilder::create(workerConfigurations)
                         .setQueryStatusListener(std::make_shared<DummyQueryListener>())
                         .build();
        bufferManager = nodeEngine->getBufferManager();
        queryManager = nodeEngine->getQueryManager();
    }

    /* Will be called after a test is executed. */
    void TearDown() override {
        Testing::BaseIntegrationTest::TearDown();
        ASSERT_TRUE(nodeEngine->stop());
        x_DEBUG("MQTTSOURCETEST::TearDown() Tear down MQTTSourceTest");
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_DEBUG("MQTTSOURCETEST::TearDownTestCases() Tear down MQTTSourceTest test class."); }

    Runtime::NodeEnginePtr nodeEngine{nullptr};
    Runtime::BufferManagerPtr bufferManager;
    Runtime::QueryManagerPtr queryManager;
    SchemaPtr test_schema;
    uint64_t buffer_size{};
    MQTTSourceTypePtr mqttSourceType;
};

/**
 * Tests basic set up of MQTT source
 */
TEST_F(MQTTSourceTest, MQTTSourceInit) {

    auto mqttSource = createMQTTSource(test_schema,
                                       bufferManager,
                                       queryManager,
                                       mqttSourceType,
                                       OPERATORID,
                                       ORIGINID,
                                       NUMSOURCELOCALBUFFERS,
                                       PHYSICALSOURCENAME,
                                       SUCCESSORS);

    SUCCEED();
}

/**
 * Test if schema, MQTT server address, clientId, user, and topic are the same
 */
TEST_F(MQTTSourceTest, MQTTSourcePrint) {

    mqttSourceType->setUrl("tcp://127.0.0.1:1883");
    mqttSourceType->setCleanSession(false);
    mqttSourceType->setClientId("x-mqtt-test-client");
    mqttSourceType->setUserName("rfRqLGZRChg8eS30PEeR");
    mqttSourceType->setTopic("v1/devices/me/telemetry");
    mqttSourceType->setQos(1);

    auto mqttSource = createMQTTSource(test_schema,
                                       bufferManager,
                                       queryManager,
                                       mqttSourceType,
                                       OPERATORID,
                                       ORIGINID,
                                       NUMSOURCELOCALBUFFERS,
                                       PHYSICALSOURCENAME,
                                       SUCCESSORS);

    std::string expected = "MQTTSOURCE(SCHEMA(var:INTEGER(32 bits)), SERVERADDRESS=tcp://127.0.0.1:1883, "
                           "CLIENTID=x-mqtt-test-client, "
                           "USER=rfRqLGZRChg8eS30PEeR, TOPIC=v1/devices/me/telemetry, "
                           "DATATYPE=JSON, QOS=atLeastOnce, CLEANSESSION=0. BUFFERFLUSHINTERVALMS=-1. ";

    EXPECT_EQ(mqttSource->toString(), expected);

    x_DEBUG("{}", mqttSource->toString());

    SUCCEED();
}

/**
 * Tests if obtained value is valid.
 */
TEST_F(MQTTSourceTest, DISABLED_MQTTSourceValue) {

    auto test_schema = Schema::create()->addField("var", BasicType::UINT32);
    auto mqttSource = createMQTTSource(test_schema,
                                       bufferManager,
                                       queryManager,
                                       mqttSourceType,
                                       OPERATORID,
                                       ORIGINID,
                                       NUMSOURCELOCALBUFFERS,
                                       PHYSICALSOURCENAME,
                                       SUCCESSORS);
    auto tuple_buffer = mqttSource->receiveData();
    EXPECT_TRUE(tuple_buffer.has_value());
    uint64_t value = 0;
    auto* tuple = (uint32_t*) tuple_buffer->getBuffer();
    value = *tuple;
    uint64_t expected = 43;
    x_DEBUG("MQTTSOURCETEST::TEST_F(MQTTSourceTest, MQTTSourceValue) expected value is: {}. Received value is: {}",
              expected,
              value);
    EXPECT_EQ(value, expected);
}

// Disabled, because it requires a manually set up MQTT broker and a data sending MQTT client
TEST_F(MQTTSourceTest, DISABLED_testDeployOneWorkerWithMQTTSourceConfig) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr wrkConf = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    wrkConf->coordinatorPort = *rpcCoordinatorPort;

    x_INFO("QueryDeploymentTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    //register logical source qnv
    std::string source =
        R"(Schema::create()->addField("type", DataTypeFactory::createArray(10, DataTypeFactory::createChar()))
                            ->addField(createField("hospitalId", BasicType::UINT64))
                            ->addField(createField("stationId", BasicType::UINT64))
                            ->addField(createField("patientId", BasicType::UINT64))
                            ->addField(createField("time", BasicType::UINT64))
                            ->addField(createField("healthStatus", BasicType::UINT8))
                            ->addField(createField("healthStatusDuration", BasicType::UINT32))
                            ->addField(createField("recovered", BasicType::BOOLEAN))
                            ->addField(createField("dead", BasicType::BOOLEAN));)";
    crd->getSourceCatalogService()->registerLogicalSource("stream", source);
    x_INFO("QueryDeploymentTest: Coordinator started successfully");

    x_INFO("QueryDeploymentTest: Start worker 1");
    wrkConf->coordinatorPort = port;
    mqttSourceType->setUrl("ws://127.0.0.1:9002");
    mqttSourceType->setClientId("testClients");
    mqttSourceType->setUserName("testUser");
    mqttSourceType->setTopic("demoCityHospital_1");
    mqttSourceType->setQos(2);
    mqttSourceType->setCleanSession(true);
    mqttSourceType->setFlushIntervalMS(2000);
    auto physicalSource = PhysicalSource::create("stream", "test_stream", mqttSourceType);
    wrkConf->physicalSources.add(physicalSource);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(wrkConf));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("QueryDeploymentTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "test.out";
    x_INFO("QueryDeploymentTest: Submit query");
    string query = R"(Query::from("stream").filter(Attribute("hospitalId") < 5).sink(FileSinkDescriptor::create(")"
        + outputFilePath + R"(", "CSV_FORMAT", "APPEND"));)";
    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    sleep(2);
    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("QueryDeploymentTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("QueryDeploymentTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("QueryDeploymentTest: Test finished");
}

TEST_F(MQTTSourceTest, DISABLED_testDeployOneWorkerWithMQTTSourceConfigTFLite) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr wrkConf = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    wrkConf->coordinatorPort = *rpcCoordinatorPort;

    x_INFO("QueryDeploymentTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    //register logical stream qnv
    std::string stream = R"(Schema::create()->addField(createField("id", BasicType::UINT64))
                                   ->addField(createField("SepalLengthCm", BasicType::FLOAT32))
                                   ->addField(createField("SepalWidthCm", BasicType::FLOAT32))
                                   ->addField(createField("PetalLengthCm", BasicType::FLOAT32))
                                   ->addField(createField("PetalWidthCm", BasicType::FLOAT32))
                                   ->addField(createField("SpeciesCode", BasicType::UINT64))
                                   ->addField(createField("CreationTime", BasicType::UINT64));)";
    crd->getSourceCatalogService()->registerLogicalSource("iris", stream);
    x_INFO("QueryDeploymentTest: Coordinator started successfully");

    x_INFO("QueryDeploymentTest: Start worker 1");
    wrkConf->coordinatorPort = port;
    mqttSourceType->setUrl("127.0.0.1:1883");
    mqttSourceType->setClientId("cpp-mqtt-iris");
    mqttSourceType->setUserName("emqx");
    mqttSourceType->setTopic("iris");
    mqttSourceType->setQos(2);
    mqttSourceType->setCleanSession(true);
    mqttSourceType->setFlushIntervalMS(2000);
    mqttSourceType->setInputFormat("CSV");
    auto physicalSource = PhysicalSource::create("iris", "iris_phys", mqttSourceType);
    wrkConf->physicalSources.add(physicalSource);
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(wrkConf));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart1);
    x_INFO("QueryDeploymentTest: Worker1 started successfully");

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    std::string outputFilePath = getTestResourceFolder() / "test.out";
    x_INFO("QueryDeploymentTest: Submit query");
    string query = R"(Query::from("iris")
        .inferModel(")"
        + std::string(TEST_DATA_DIRECTORY) + R"(iris_95acc.tflite",
                {Attribute("SepalLengthCm"), Attribute("SepalWidthCm"), Attribute("PetalLengthCm"), Attribute("PetalWidthCm")},
                {Attribute("iris0", BasicType::FLOAT32), Attribute("iris1", BasicType::FLOAT32), Attribute("iris2", BasicType::FLOAT32)})
        .filter((Attribute("iris0") > Attribute("iris1") && Attribute("iris0") > Attribute("iris2") && Attribute("SpeciesCode") > 0) ||
                (Attribute("iris1") > Attribute("iris0") && Attribute("iris1") > Attribute("iris2") && (Attribute("SpeciesCode") < 1 || Attribute("SpeciesCode") > 1)) ||
                (Attribute("iris2") > Attribute("iris0") && Attribute("iris2") > Attribute("iris1") && Attribute("SpeciesCode") < 2), 0.1)
        .sink(FileSinkDescriptor::create(")"
        + outputFilePath + R"(", "CSV_FORMAT", "APPEND"));)";
    QueryId queryId = queryService->validateAndQueueAddQueryRequest(query,
                                                                    Optimizer::PlacementStrategy::BottomUp,
                                                                    FaultToleranceType::NONE,
                                                                    LineageType::IN_MEMORY);
    GlobalQueryPlanPtr globalQueryPlan = crd->getGlobalQueryPlan();
    EXPECT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));
    sleep(10);

    x_INFO("\n\n --------- CONTENT --------- \n\n");
    std::ifstream ifs(outputFilePath);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    x_INFO("{}", content);

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    EXPECT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    x_INFO("QueryDeploymentTest: Stop worker 1");
    bool retStopWrk1 = wrk1->stop(true);
    EXPECT_TRUE(retStopWrk1);

    x_INFO("QueryDeploymentTest: Stop Coordinator");
    bool retStopCord = crd->stopCoordinator(true);
    EXPECT_TRUE(retStopCord);
    x_INFO("QueryDeploymentTest: Test finished");
}

}// namespace x
#endif
