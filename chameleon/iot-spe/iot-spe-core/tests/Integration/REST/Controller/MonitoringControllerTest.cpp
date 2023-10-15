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

#include <API/Query.hpp>
#include <BaseIntegrationTest.hpp>
#include <GRPC/WorkerRPCClient.hpp>
#include <Monitoring/MetricCollectors/MetricCollectorType.hpp>
#include <Monitoring/MonitoringManager.hpp>
#include <Monitoring/MonitoringPlan.hpp>
#include <Monitoring/ResourcesReader/SystemResourcesReaderFactory.hpp>
#include <Runtime/BufferManager.hpp>
#include <Services/MonitoringService.hpp>
#include <Services/QueryParsingService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/MetricValidator.hpp>
#include <cpr/cpr.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace x {

class MonitoringControllerTest : public Testing::BaseIntegrationTest {
  public:
    Runtime::BufferManagerPtr bufferManager;
    uint16_t timeout = 15;
    static void SetUpTestCase() {
        x::Logger::setupLogging("MonitoringControllerTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup MonitoringControllerTest test class.");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();
        bufferManager = std::make_shared<Runtime::BufferManager>(4096, 10);
        x_INFO("Setup MonitoringControllerTest test class.");
    }

    bool waitForMonitoringQuery(xCoordinatorPtr coordinator, ::uint64_t restPort, ::uint64_t timeout) {
        std::chrono::seconds timeoutInSec = std::chrono::seconds(timeout);
        nlohmann::json monitoringQueries = coordinator->getMonitoringService()->getMonitoringStreams();
        if (!monitoringQueries.is_array()) {
            return false;
        }
        auto start_timestamp = std::chrono::system_clock::now();
        while (std::chrono::system_clock::now() < start_timestamp + timeoutInSec) {
            monitoringQueries = coordinator->getMonitoringService()->getMonitoringStreams();
            if (!monitoringQueries.is_array()) {
                return false;
            }
            if (monitoringQueries.size() >= 1) {
                break;
            }
        }
        start_timestamp = std::chrono::system_clock::now();
        bool succes = false;
        while (!succes && std::chrono::system_clock::now() < start_timestamp + timeoutInSec) {
            succes = true;
            for (auto& query : monitoringQueries) {
                auto id = query["query_ID"].get<uint>();
                x_DEBUG("checking status of query {}", id);
                if (!TestUtils::checkRunningOrTimeout(id, std::to_string(restPort))) {
                    succes = false;
                }
            }
        }
        return succes;
    }

    bool waitForMonitoringQueryEnd(xCoordinatorPtr coordinator, ::uint64_t timeout) {
        std::chrono::seconds timeoutInSec = std::chrono::seconds(timeout);
        nlohmann::json monitoringQueries = coordinator->getMonitoringService()->getMonitoringStreams();
        if (!monitoringQueries.is_array()) {
            return false;
        }
        auto start_timestamp = std::chrono::system_clock::now();
        while (std::chrono::system_clock::now() < start_timestamp + timeoutInSec) {
            monitoringQueries = coordinator->getMonitoringService()->getMonitoringStreams();
            if (!monitoringQueries.is_array()) {
                return false;
            }
            if (monitoringQueries.size() == 0) {
                return true;
            }
        }
        return false;
    }

    static void TearDownTestCase() { x_INFO("Tear down MonitoringControllerTest test class."); }
};

TEST_F(MonitoringControllerTest, testStartMonitoring) {
    x_INFO("Tests for Oatpp Monitoring Controller start monitoring: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    // oatpp GET start call
    cpr::Response r = cpr::Get(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/start"});
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));

    //check if content of r contains valid information
    std::set<std::string> expectedMonitoringStreams{toString(Monitoring::MetricType::WrappedNetworkMetrics),
                                                    toString(Monitoring::MetricType::WrappedCpuMetrics),
                                                    toString(Monitoring::MetricType::MemoryMetric),
                                                    toString(Monitoring::MetricType::DiskMetric)};

    nlohmann::json jsonsStart;
    ASSERT_NO_THROW(jsonsStart = nlohmann::json::parse(r.text));
    x_INFO("MonitoringControllerTest - Received Data from GetStart request: {}", jsonsStart.dump());
    ASSERT_EQ(jsonsStart.size(), expectedMonitoringStreams.size());
    bool check = MetricValidator::checkEntriesOfStream(expectedMonitoringStreams, jsonsStart);
    ASSERT_TRUE(check);
    ASSERT_TRUE(waitForMonitoringQuery(coordinator, coordinatorConfig->restPort.getValue(), 5));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testStopMonitoring) {
    x_INFO("Tests for Oatpp Monitoring Controller start monitoring: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    cpr::Response r = cpr::Get(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/stop"});
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testStartMonitoringFailsBecauseMonitoringIsNotEnabled) {
    x_INFO("Tests for Oatpp Monitoring Controller start monitoring: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    auto future = cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/start"});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 500);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testStopMonitoringFailsBecauseMonitoringIsNotEnabled) {
    x_INFO("Tests for Oatpp Monitoring Controller: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    auto future = cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/stop"});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 500);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testRequestAllMetrics) {
    x_INFO("Tests for Oatpp Monitoring Controller - testRequestAllMetrics: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }

    auto future = cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/metrics"});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));

    nlohmann::json jsonsOfResponse;
    ASSERT_NO_THROW(jsonsOfResponse = nlohmann::json::parse(r.text));
    x_INFO("MonitoringControllerTest - Received Data from GetAllMetrics request: {}", jsonsOfResponse);

    //check if content of r contains valid information (right fields and valid queryIds):
    ASSERT_EQ(jsonsOfResponse.size(), 1);
    auto json = jsonsOfResponse[std::to_string(1)];
    x_INFO("MonitoringControllerTest: Requesting monitoring data from node with ID {}", std::to_string(1));
    x_INFO("Received Data for node 1: {}", json.dump());
    ASSERT_TRUE(MetricValidator::isValidAll(Monitoring::SystemResourcesReaderFactory::getSystemResourcesReader(), json));
    ASSERT_TRUE(MetricValidator::checkNodeIds(json, 1));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testGetMonitoringControllerDataFromOneNode) {
    x_INFO("Tests for Oatpp Monitoring Controller - testGetMonitoringControllerDataFromOneNode: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    cpr::AsyncResponse future =
        cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/metrics"},
                      cpr::Parameters{{"nodeId", std::to_string(1)}});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    nlohmann::json jsonsOfResponse;
    ASSERT_NO_THROW(jsonsOfResponse = nlohmann::json::parse(r.text));
    x_INFO("MonitoringControllerTest - Received Data from GetMetricsForOneNode request: {}", jsonsOfResponse);

    //check if content of r contains valid information (right fields and valid queryIds):
    ASSERT_EQ(jsonsOfResponse.size(), 1);
    auto json = jsonsOfResponse[std::to_string(1)];
    x_INFO("MonitoringControllerTest: Requesting monitoring data from node with ID {}", std::to_string(1));
    ASSERT_TRUE(MetricValidator::isValidAll(Monitoring::SystemResourcesReaderFactory::getSystemResourcesReader(), json));
    ASSERT_TRUE(MetricValidator::checkNodeIds(json, 1));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testGetMonitoringControllerStorage) {
    x_INFO("Tests for Oatpp Monitoring Controller - testGetMonitoringControllerStorage: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    MonitoringServicePtr monitoringService = coordinator->getMonitoringService();
    auto expected = monitoringService->requestNewestMonitoringDataFromMetricStoreAsJson();
    cpr::AsyncResponse future =
        cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/storage"});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    //compare content of response to expected values
    nlohmann::json jsons;
    ASSERT_NO_THROW(jsons = nlohmann::json::parse(r.text));
    ASSERT_EQ(jsons.size(), 1);
    x_INFO("MonitoringControllerTest - Received Data from Get-Storage request: {}", jsons);
    auto json = jsons[std::to_string(1)];
    x_INFO("MonitoringControllerTest: Requesting monitoring data from node with ID {}", std::to_string(1));
    x_INFO("Received Data for node 1: {}", json.dump());
    auto jsonRegistration = json[toString(Monitoring::MetricType::RegistrationMetric)][0]["value"];
    ASSERT_TRUE(MetricValidator::isValidRegistrationMetrics(Monitoring::SystemResourcesReaderFactory::getSystemResourcesReader(),
                                                            jsonRegistration));
    ASSERT_EQ(jsonRegistration["NODE_ID"], 1);
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(MonitoringControllerTest, testGetMonitoringControllerStreams) {
    x_INFO("Tests for Oatpp Monitoring Controller - testGetMonitoringControllerStreams: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    coordinatorConfig->enableMonitoring = true;
    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    ASSERT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("MonitoringControllerTest: Coordinator started successfully");
    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "Rest server failed to start";
    }
    MonitoringServicePtr monitoringService = coordinator->getMonitoringService();
    auto expected = monitoringService->startMonitoringStreams();
    cpr::AsyncResponse future =
        cpr::GetAsync(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/monitoring/streams"});
    future.wait();
    auto r = future.get();
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));

    //compare content of response to expected values
    nlohmann::json jsons;
    ASSERT_NO_THROW(jsons = nlohmann::json::parse(r.text));
    x_INFO("MonitoringControllerTest - Received Data from Get-Streams request: {}", jsons);
    std::set<std::string> expectedMonitoringStreams{toString(Monitoring::MetricType::WrappedNetworkMetrics),
                                                    toString(Monitoring::MetricType::WrappedCpuMetrics),
                                                    toString(Monitoring::MetricType::MemoryMetric),
                                                    toString(Monitoring::MetricType::DiskMetric)};
    bool check = MetricValidator::checkEntriesOfStream(expectedMonitoringStreams, jsons);
    ASSERT_TRUE(check);
    ASSERT_TRUE(waitForMonitoringQuery(coordinator, coordinatorConfig->restPort.getValue(), 5));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

}//namespace x
