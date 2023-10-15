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
#include <gtest/gtest.h>//

#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger/Logger.hpp>

#include <BaseIntegrationTest.hpp>

using namespace std;
namespace x {

class UpdateTopologyRemoteTest : public Testing::BaseIntegrationTest {
  public:
    // set the default numberOfSlots to the number of processor
    const uint16_t processorCount = std::thread::hardware_concurrency();
    std::string ipAddress = "127.0.0.1";
    uint16_t coordinatorNumberOfSlots = 12;
    uint16_t workerNumberOfSlots = 6;

    static void SetUpTestCase() {
        x::Logger::setupLogging("UpdateTopologyRemoteTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup UpdateTopologyRemoteTest test class.");
    }

    static void TearDownTestCase() { x_DEBUG("Tear down UpdateTopologyRemoteTest test class."); }
};

TEST_F(UpdateTopologyRemoteTest, addAndRemovePathWithOwnId) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    auto workerConfig1 = WorkerConfiguration::create();
    auto workerConfig2 = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    coordinatorConfig->worker.numberOfSlots = (coordinatorNumberOfSlots);

    workerConfig1->numberOfSlots = (workerNumberOfSlots);
    workerConfig2->numberOfSlots = (workerNumberOfSlots);

    coordinatorConfig->worker.numberOfSlots = (coordinatorNumberOfSlots);

    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    auto port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0ull);
    x_INFO("coordinator started successfully");

    x_INFO("start worker");
    auto node1RpcPort = getAvailablePort();
    auto node1DataPort = getAvailablePort();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;
    workerConfig1->rpcPort = (*node1RpcPort);
    workerConfig1->dataPort = (*node1DataPort);
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart);
    x_INFO("worker started successfully");

    auto node2RpcPort = getAvailablePort();
    auto node2DataPort = getAvailablePort();
    workerConfig2->coordinatorPort = *rpcCoordinatorPort;
    workerConfig2->rpcPort = (*node2RpcPort);
    workerConfig2->dataPort = (*node2DataPort);
    xWorkerPtr wrk2 = std::make_shared<xWorker>(std::move(workerConfig2));
    bool retStart2 = wrk2->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart2);
    x_INFO("worker started successfully");

    TopologyPtr topology = crd->getTopology();
    EXPECT_TRUE(topology->nodeWithWorkerIdExists(1u));// worker inside the coordinator
    EXPECT_TRUE(topology->nodeWithWorkerIdExists(2u));// first worker should get id 2
    EXPECT_TRUE(topology->nodeWithWorkerIdExists(3u));// second worker should get id 3

    TopologyNodePtr rootNode = topology->getRoot();
    //    EXPECT_TRUE(rootNode->getGrpcPort() == *rpcCoordinatorPort);
    EXPECT_TRUE(rootNode->getChildren().size() == 2);
    EXPECT_TRUE(rootNode->getAvailableResources() == coordinatorNumberOfSlots);
    TopologyNodePtr node1 = rootNode->getChildren()[0]->as<TopologyNode>();
    EXPECT_TRUE(node1->getGrpcPort() == *node1RpcPort);
    EXPECT_TRUE(node1->getAvailableResources() == workerNumberOfSlots);
    TopologyNodePtr node2 = rootNode->getChildren()[1]->as<TopologyNode>();
    EXPECT_TRUE(node2->getGrpcPort() == *node2RpcPort);
    EXPECT_TRUE(node2->getAvailableResources() == workerNumberOfSlots);

    x_INFO("ADD NEW PARENT");
    bool successAddPar = wrk->addParent(node2->getId());
    EXPECT_TRUE(successAddPar);
    EXPECT_TRUE(rootNode->getChildren().size() == 2);
    EXPECT_TRUE(node2->getChildren().size() == 1);
    EXPECT_TRUE(node2->getChildren()[0]->as<TopologyNode>()->getId() == node1->getId());

    x_INFO("REMOVE NEW PARENT");
    bool successRemoveParent = wrk->removeParent(node2->getId());
    EXPECT_TRUE(successRemoveParent);
    EXPECT_TRUE(successAddPar);
    EXPECT_TRUE(rootNode->getChildren().size() == 2);
    EXPECT_TRUE(node2->getChildren().empty());

    x_INFO("stopping worker");
    bool retStopWrk = wrk->stop(false);
    EXPECT_TRUE(retStopWrk);

    x_INFO("stopping worker 2");
    bool retStopWrk2 = wrk2->stop(false);
    EXPECT_TRUE(retStopWrk2);

    x_INFO("stopping coordinator");
    bool retStopCord = crd->stopCoordinator(false);
    EXPECT_TRUE(retStopCord);
}

TEST_F(UpdateTopologyRemoteTest, addAndRemovePathWithOwnIdAndSelf) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    auto workerConfig1 = WorkerConfiguration::create();
    auto workerConfig2 = WorkerConfiguration::create();

    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    coordinatorConfig->worker.numberOfSlots = (coordinatorNumberOfSlots);
    workerConfig1->numberOfSlots = (workerNumberOfSlots);
    workerConfig2->numberOfSlots = (workerNumberOfSlots);

    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("coordinator started successfully");

    x_INFO("start worker");
    auto node1RpcPort = getAvailablePort();
    auto node1DataPort = getAvailablePort();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;
    workerConfig1->rpcPort = (*node1RpcPort);
    workerConfig1->dataPort = (*node1DataPort);
    xWorkerPtr wrk = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart = wrk->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart);
    x_INFO("worker started successfully");

    auto node2RpcPort = getAvailablePort();
    auto node2DataPort = getAvailablePort();
    workerConfig2->coordinatorPort = *rpcCoordinatorPort;
    workerConfig2->rpcPort = (*node2RpcPort);
    workerConfig2->dataPort = (*node2DataPort);
    xWorkerPtr wrk2 = std::make_shared<xWorker>(std::move(workerConfig2));
    bool retStart2 = wrk2->start(/**blocking**/ false, /**withConnect**/ true);
    EXPECT_TRUE(retStart2);
    x_INFO("worker started successfully");

    TopologyPtr topology = crd->getTopology();

    TopologyNodePtr rootNode = topology->getRoot();
    //    EXPECT_TRUE(rootNode->getGrpcPort() == *rpcCoordinatorPort);
    EXPECT_TRUE(rootNode->getChildren().size() == 2);
    TopologyNodePtr node1 = rootNode->getChildren()[0]->as<TopologyNode>();
    EXPECT_TRUE(node1->getGrpcPort() == *node1RpcPort);
    TopologyNodePtr node2 = rootNode->getChildren()[1]->as<TopologyNode>();
    EXPECT_TRUE(node2->getGrpcPort() == *node2RpcPort);

    x_INFO("REMOVE NEW PARENT");
    bool successRemoveParent = wrk->removeParent(node1->getId());
    EXPECT_TRUE(!successRemoveParent);

    x_INFO("stopping worker");
    bool retStopWrk = wrk->stop(false);
    EXPECT_TRUE(retStopWrk);

    x_INFO("stopping worker 2");
    bool retStopWrk2 = wrk2->stop(false);
    EXPECT_TRUE(retStopWrk2);

    x_INFO("stopping coordinator");
    bool retStopCord = crd->stopCoordinator(false);
    EXPECT_TRUE(retStopCord);
}

}// namespace x
