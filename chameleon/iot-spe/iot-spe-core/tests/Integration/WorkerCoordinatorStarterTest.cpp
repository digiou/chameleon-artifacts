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

#include <iostream>

#include <BaseIntegrationTest.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>

using std::cout;
using std::endl;
#define DEBUG_OUTPUT
namespace x {

class WorkerCoordinatorStarterTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("WorkerCoordinatorStarterTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup WorkerCoordinatorStarterTest test class.");
    }
};

TEST_F(WorkerCoordinatorStarterTest, startStopWorkerCoordinator) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;

    x_INFO("start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);

    ASSERT_NE(port, 0ULL);
    x_INFO("coordinator started successfully");

    x_INFO("start worker");
    workerConfig->coordinatorPort = port;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart);
    x_INFO("worker started connected successfully");

    x_INFO("stopping worker");
    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);

    cout << "stopping coordinator" << endl;
    bool retStopCord = crd->stopCoordinator(false);
    cout << crd.use_count() << " use cnt coord" << endl;
    ASSERT_TRUE(retStopCord);
}

TEST_F(WorkerCoordinatorStarterTest, startStopWorkerCoordinator10times) {
    cout << "start coordinator" << endl;
    for (uint64_t i = 0; i < 10; i++) {
        CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
        WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

        coordinatorConfig->rpcPort = *rpcCoordinatorPort;
        coordinatorConfig->restPort = *restPort;
        workerConfig->coordinatorPort = *rpcCoordinatorPort;
        cout << "iteration = " << i << endl;
        xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
        uint64_t port = crd->startCoordinator(/**blocking**/ false);
        ASSERT_NE(port, 0ULL);
        cout << "coordinator started successfully" << endl;

        cout << "start worker" << endl;
        workerConfig->coordinatorPort = port;
        xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
        bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ true);
        ASSERT_TRUE(retStart);
        cout << "worker started connected successfully" << endl;

        cout << "wakeup" << endl;

        cout << "stopping worker" << endl;
        bool retStopWrk = wrk->stop(false);
        ASSERT_TRUE(retStopWrk);

        cout << "stopping coordinator" << endl;
        bool retStopCord = crd->stopCoordinator(false);
        cout << crd.use_count() << " use cnt" << endl;
        cout << wrk.use_count() << " use cnt" << endl;
        crd.reset();
        wrk.reset();
        cout << crd.use_count() << " use cnt" << endl;
        cout << wrk.use_count() << " use cnt" << endl;
        ASSERT_TRUE(retStopCord);
    }
}
TEST_F(WorkerCoordinatorStarterTest, startStopCoordinatorWorker) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;

    cout << "start coordinator" << endl;
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0ULL);
    cout << "coordinator started successfully" << endl;

    cout << "start worker" << endl;
    workerConfig->coordinatorPort = port;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart);
    cout << "worker started connected successfully" << endl;

    cout << "wakeup" << endl;

    cout << "stopping coordinator" << endl;
    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);

    cout << "stopping worker" << endl;
    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);
}

TEST_F(WorkerCoordinatorStarterTest, startConnectStopWorkerCoordinator) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;

    cout << "start coordinator" << endl;
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0ULL);
    cout << "coordinator started successfully" << endl;

    cout << "start worker" << endl;
    workerConfig->coordinatorPort = port;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ false);
    ASSERT_TRUE(retStart);
    cout << "worker started successfully" << endl;

    bool retConWrk = wrk->connect();
    ASSERT_TRUE(retConWrk);
    cout << "worker got connected " << endl;

    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(WorkerCoordinatorStarterTest, startConnectStopWithoutDisconnectWorkerCoordinator) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;

    cout << "start coordinator" << endl;
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0ULL);
    cout << "coordinator started successfully" << endl;

    cout << "start worker" << endl;
    workerConfig->coordinatorPort = port;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ false);
    ASSERT_TRUE(retStart);
    cout << "worker started successfully" << endl;

    bool retConWrk = wrk->connect();
    ASSERT_TRUE(retConWrk);
    cout << "worker got connected " << endl;

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);

    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);
}

TEST_F(WorkerCoordinatorStarterTest, startConnectDisconnectStopWorkerCoordinator) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    coordinatorConfig->coordinatorHealthCheckWaitTime = 1;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;

    cout << "start coordinator" << endl;
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0ULL);
    cout << "coordinator started successfully" << endl;

    cout << "start worker" << endl;
    workerConfig->coordinatorPort = port;
    workerConfig->workerHealthCheckWaitTime = 1;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ false);
    ASSERT_TRUE(retStart);
    cout << "worker started successfully" << endl;

    bool retConWrk = wrk->connect();
    ASSERT_TRUE(retConWrk);
    cout << "worker got connected " << endl;

    bool retDisWrk = wrk->disconnect();
    ASSERT_TRUE(retDisWrk);
    cout << "worker got disconnected " << endl;

    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

TEST_F(WorkerCoordinatorStarterTest, startReconnectStopWorkerCoordinator) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    WorkerConfigurationPtr workerConfig = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    coordinatorConfig->coordinatorHealthCheckWaitTime = 1;
    workerConfig->coordinatorPort = *rpcCoordinatorPort;
    workerConfig->workerHealthCheckWaitTime = 1;

    cout << "start coordinator" << endl;
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    ASSERT_NE(port, 0ULL);
    cout << "coordinator started successfully" << endl;

    cout << "start worker" << endl;
    workerConfig->coordinatorPort = port;
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ false);
    ASSERT_TRUE(retStart);
    cout << "worker started successfully" << endl;

    bool retConWrk = wrk->connect();
    ASSERT_TRUE(retConWrk);
    cout << "worker got connected " << endl;

    bool retDisWrk = wrk->disconnect();
    ASSERT_TRUE(retDisWrk);
    cout << "worker got disconnected " << endl;

    bool retConWrk2 = wrk->connect();
    ASSERT_TRUE(retConWrk2);
    cout << "worker got connected " << endl;

    bool retDisWrk2 = wrk->disconnect();
    ASSERT_TRUE(retDisWrk2);
    cout << "worker got disconnected " << endl;
    bool retStopWrk = wrk->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}
}// namespace x
