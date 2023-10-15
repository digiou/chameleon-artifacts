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
#include <REST/ServerTypes.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestUtils.hpp>
#include <cpr/cpr.h>
#include <gtest/gtest.h>
#include <memory>

namespace x {
class ConnectivityControllerTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("ConnectivityControllerTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup TopologyControllerTest test class.");
    }

    static void TearDownTestCase() { x_INFO("Tear down ConnectivityControllerTest test class."); }
};

TEST_F(ConnectivityControllerTest, testCORSRequest) {
    x_INFO("TestsForOatppEndpoints: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    std::string corsOrigin = "https://www.IoTSPE.stream";
    coordinatorConfig->restServerCorsAllowedOrigin = corsOrigin;

    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    EXPECT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("ConnectivityControllerTest: Coordinator started successfully");

    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "REST Server failed to start";
    }
    cpr::Response r = cpr::Get(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/connectivity/check"});
    EXPECT_EQ(r.status_code, 200l);
    std::string corsOriginHeader;
    EXPECT_NO_THROW(corsOriginHeader = r.header.at("Access-Control-Allow-Origin"));
    EXPECT_EQ(corsOriginHeader, corsOrigin);
    std::string corsMethodHeader;
    EXPECT_NO_THROW(corsMethodHeader = r.header.at("Access-Control-Allow-Methods"));
    EXPECT_EQ(corsMethodHeader, "GET, POST, OPTIONS");
    std::string corsAllowedHeaders;
    EXPECT_NO_THROW(corsAllowedHeaders = r.header.at("Access-Control-Allow-Headers"));
    EXPECT_EQ(corsAllowedHeaders,
              "DNT, User-Agent, X-Requested-With, If-Modified-Since, Cache-Control, Content-Type, Range, Authorization");
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

TEST_F(ConnectivityControllerTest, testGetRequest) {
    x_INFO("TestsForOatppEndpoints: Start coordinator");
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;

    auto coordinator = std::make_shared<xCoordinator>(coordinatorConfig);
    EXPECT_EQ(coordinator->startCoordinator(false), *rpcCoordinatorPort);
    x_INFO("ConnectivityControllerTest: Coordinator started successfully");

    bool success = TestUtils::checkRESTServerStartedOrTimeout(coordinatorConfig->restPort.getValue(), 5);
    if (!success) {
        FAIL() << "REST Server failed to start";
    }
    cpr::Response r = cpr::Get(cpr::Url{"http://127.0.0.1:" + std::to_string(*restPort) + "/v1/x/connectivity/check"});
    EXPECT_EQ(r.status_code, 200l);
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Origin"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Methods"));
    EXPECT_FALSE(r.header.contains("Access-Control-Allow-Headers"));
    bool stopCrd = coordinator->stopCoordinator(true);
    ASSERT_TRUE(stopCrd);
}

}//namespace x