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
#include <iostream>    // For cout
#include <netinet/in.h>// For sockaddr_in
#include <sys/socket.h>// For socket functions
#include <unistd.h>    // For read

#include <Catalogs/Query/QueryCatalog.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/TCPSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Services/QueryService.hpp>
#include <Sinks/Mediums/NullOutputSink.hpp>
#include <Sources/DataSource.hpp>
#include <Sources/TCPSource.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>

#include <Util/TestUtils.hpp>
#include <thread>

namespace x {

class TCPSourceIntegrationTest : public Testing::BaseIntegrationTest {
  public:
    /**
     * @brief Set up test cases, starts a TCP server before all tests are run
     */
    static void SetUpTestCase() {
        x::Logger::setupLogging("TCPSourceIntegrationTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup TCPSourceIntegrationTest test class.");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();
        x_TRACE("TCPSourceIntegrationTest: Start TCPServer.");
        tcpServerPort = getAvailablePort();
        startServer();
    }

    void TearDown() override {
        stopServer();
        x_TRACE("TCPSourceIntegrationTest: Stop TCPServer.");
        Testing::BaseIntegrationTest::TearDown();
    }

    /**
     * @brief starts a TCP server on tcpServerPort
     */
    void startServer() {
        // Create a socket (IPv4, TCP)
        // After one test was executed, the remaining test calls to create a socket initially return 0
        // The close method does not recognize 0 as a valid file descriptor and therefore will not close the socket properly, hence
        // we use a while loop to retrieve a valid file descriptor
        uint8_t counter = 0;
        while (sockfd == 0 && counter < 10) {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            x_TRACE("Retrieved sockfd: {} on try: {}", sockfd, std::to_string(counter));
            counter++;
        }
        if (sockfd == 0) {
            x_ERROR("Failed to grap a valid file descriptor. errno: {}", errno);
            exit(EXIT_FAILURE);
        }
        if (sockfd == -1) {
            x_ERROR("Failed to create socket. errno: {}", errno);
            exit(EXIT_FAILURE);
        }

        // Listen to port tcpServerPort on any address
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        sockaddr.sin_port = htons(*tcpServerPort);// htons is necessary to convert a number to
                                                  // network byte order
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            x_ERROR("TCPSourceIntegrationTest: Failed to create socket. errno: {}", errno);
            exit(EXIT_FAILURE);
        }
        if (bind(sockfd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
            std::stringstream tcpServerPortAsString;
            tcpServerPortAsString << tcpServerPort;
            x_ERROR("TCPSourceIntegrationTest: Failed to bind to port {}. errno: {}", tcpServerPortAsString.str(), errno);
            exit(EXIT_FAILURE);
        }

        // Start listening. Hold at most 10 connections in the queue
        if (listen(sockfd, 10) < 0) {
            x_ERROR("TCPSourceIntegrationTest: Failed to listen on socket. errno: {}", errno);
            exit(EXIT_FAILURE);
        }
        x_TRACE("TCPSourceIntegrationTest: TCPServer successfully started.");
    }

    /**
     * @brief stops the TCP server running on tcpServerPort
     */
    void stopServer() {
        // Close the connections
        auto closed = close(sockfd);
        x_TRACE("Closing Server connection pls wait ...{}", closed);
        if (closed == -1) {
            x_ERROR("TCPSourceIntegrationTest::stopServer: Could not close socket. {}", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief sends multiple messages via TCP connection. Static because this is run in a thread inside the test cases
     * @param message message as string to be send
     * @param repeatSending how of the message should be send
     */
    int sendMessages(std::string message, int repeatSending) {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        int connection = accept(sockfd, (struct sockaddr*) &sockaddr, (socklen_t*) &addrlen);
        if (connection < 0) {
            x_ERROR("TCPSourceIntegrationTest: Failed to grab connection. errno: {}", strerror(errno));
            return -1;
        }
        for (int i = 0; i < repeatSending; ++i) {
            x_TRACE("TCPSourceIntegrationTest: Sending message: {} iter={}", message, i);
            send(connection, message.c_str(), message.size(), 0);
        }
        // Close the connections
        return close(connection);
    }

    /**
     * @brief sending different comma seperated messages via TCP to test variable length read. Static because it is run in a
     * threat inside test cases
     */
    int sendMessageCSVVariableLength() {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        int connection = accept(sockfd, (struct sockaddr*) &sockaddr, (socklen_t*) &addrlen);
        if (connection < 0) {
            x_ERROR("TCPSourceIntegrationTest: Failed to grab connection. errno: {}", strerror(errno));
            return -1;
        }
        std::string message = "100,4.986,sonne";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "192,4.96,sonne";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "130,4.9,stern";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "589,4.98621,sonne";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "39,4.198,malen";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "102,9.986,hello";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        // Close the connections
        return close(connection);
    }

    /**
     * @brief sending different JSON messages via TCP to test variable length read. Static because it is run in a
     * threat inside test cases
     */
    int sendMessageJSONVariableLength() {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        int connection = accept(sockfd, (struct sockaddr*) &sockaddr, (socklen_t*) &addrlen);
        if (connection < 0) {
            x_ERROR("TCPSourceIntegrationTest: Failed to grab connection. errno: {}", strerror(errno));
            return -1;
        }
        std::string message = "{\"id\":\"4\", \"value\":\"5.893\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "{\"id\":\"8\", \"value\":\"5.8939\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "{\"id\":\"432\", \"value\":\"5.83\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "{\"id\":\"99\", \"value\":\"0.893\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "{\"id\":\"911\", \"value\":\"5.8893\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        message = "{\"id\":\"4293\", \"value\":\"5.89311\", \"name\":\"hello\"}";
        x_TRACE("TCPSourceIntegrationTest: Sending message: {}", message);
        send(connection, std::to_string(message.length()).c_str(), 2, 0);
        send(connection, message.c_str(), message.size(), 0);

        // Close the connections
        return close(connection);
    }

    int sockfd = 0;
    sockaddr_in sockaddr = {};
    Testing::BorrowedPortPtr tcpServerPort;
};

/**
 * @brief tests TCPSource read of csv data that is seperated by a given token. Here \n is used
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadCSVDataWithSeparatorToken) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("onTime", BasicType::BOOLEAN);

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::CSV);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::TUPLE_SEPARATOR);
    sourceConfig->setTupleSeparator('\n');

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 30;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("42,5.893,true\n", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$onTime:Boolean\n"
                             "42,5.893000,1\n"
                             "42,5.893000,1\n"
                             "42,5.893000,1\n"
                             "42,5.893000,1\n"
                             "42,5.893000,1\n"
                             "42,5.893000,1\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of JSON data that is seperated by a given token. Here \n is used
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadJSONDataWithSeparatorToken) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::JSON);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::TUPLE_SEPARATOR);
    sourceConfig->setTupleSeparator('\n');

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("{\"id\":\"42\", \"value\":\"5.893\", \"name\":\"hello\"}\n", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of CSV data when obtaining the size of the data from the socket. Constant length
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadCSVDataLengthFromSocket) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::CSV);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::BUFFER_SIZE_FROM_SOCKET);
    sourceConfig->setBytesUsedForSocketBufferSizeTransfer(2);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("1442,5.893,hello", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of CSV data when obtaining the size of the data from the socket. Variable length
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadCSVWithVariableLength) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::CSV);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::BUFFER_SIZE_FROM_SOCKET);
    sourceConfig->setBytesUsedForSocketBufferSizeTransfer(2);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessageCSVVariableLength();
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "100,4.986000,sonne\n"
                             "192,4.960000,sonne\n"
                             "130,4.900000,stern\n"
                             "589,4.986210,sonne\n"
                             "39,4.198000,malen\n"
                             "102,9.986000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of JSON data when obtaining the size of the data from the socket. Constant length
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadJSONDataLengthFromSocket) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(10000);
    sourceConfig->setInputFormat(Configurations::InputFormat::JSON);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::BUFFER_SIZE_FROM_SOCKET);
    sourceConfig->setBytesUsedForSocketBufferSizeTransfer(2);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("44{\"id\":\"42\", \"value\":\"5.893\", \"name\":\"hello\"}", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of CSV data when obtaining the size of the data from the socket. Variable length
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadJSONDataWithVariableLength) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::JSON);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::BUFFER_SIZE_FROM_SOCKET);
    sourceConfig->setBytesUsedForSocketBufferSizeTransfer(2);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessageJSONVariableLength();
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "4,5.893000,hello\n"
                             "8,5.893900,hello\n"
                             "432,5.830000,hello\n"
                             "99,0.893000,hello\n"
                             "911,5.889300,hello\n"
                             "4293,5.893110,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of CSV data with fixed length inputted at source creation time
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadCSVDataWithFixedSize) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::CSV);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::USER_SPECIFIED_BUFFER_SIZE);
    sourceConfig->setSocketBufferSize(14);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("42,5.893,hello", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

/**
 * @brief tests TCPSource read of CSV data with fixed length inputted at source creation time
 */
TEST_F(TCPSourceIntegrationTest, TCPSourceReadJSONDataWithFixedSize) {
    CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::createDefault();
    coordinatorConfig->rpcPort = *rpcCoordinatorPort;
    coordinatorConfig->restPort = *restPort;
    x_INFO("TCPSourceIntegrationTest: Start coordinator");
    xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);
    uint64_t port = crd->startCoordinator(/**blocking**/ false);
    EXPECT_NE(port, 0UL);
    x_INFO("TCPSourceIntegrationTest: Coordinator started successfully");

    auto tcpSchema = Schema::create()
                         ->addField("id", BasicType::UINT32)
                         ->addField("value", BasicType::FLOAT32)
                         ->addField("name", DataTypeFactory::createFixedChar(5));

    crd->getSourceCatalogService()->registerLogicalSource("tcpStream", tcpSchema);
    x_DEBUG("TCPSourceIntegrationTest: Added tcpLogicalSource to coordinator.")

    x_DEBUG("TCPSourceIntegrationTest: Start worker 1");
    WorkerConfigurationPtr workerConfig1 = WorkerConfiguration::create();
    workerConfig1->coordinatorPort = *rpcCoordinatorPort;

    TCPSourceTypePtr sourceConfig = TCPSourceType::create();
    sourceConfig->setSocketPort(*tcpServerPort);
    sourceConfig->setSocketHost("127.0.0.1");
    sourceConfig->setSocketDomainViaString("AF_INET");
    sourceConfig->setSocketTypeViaString("SOCK_STREAM");
    sourceConfig->setFlushIntervalMS(5000);
    sourceConfig->setInputFormat(Configurations::InputFormat::JSON);
    sourceConfig->setDecideMessageSize(Configurations::TCPDecideMessageSize::USER_SPECIFIED_BUFFER_SIZE);
    sourceConfig->setSocketBufferSize(44);

    auto physicalSource = PhysicalSource::create("tcpStream", "tcpStream", sourceConfig);
    workerConfig1->physicalSources.add(physicalSource);
    workerConfig1->bufferSizeInBytes = 50;
    xWorkerPtr wrk1 = std::make_shared<xWorker>(std::move(workerConfig1));
    bool retStart1 = wrk1->start(/**blocking**/ false, /**withConnect**/ true);
    ASSERT_TRUE(retStart1);
    x_INFO("TCPSourceIntegrationTest: Worker1 started successfully");

    std::string filePath = getTestResourceFolder() / "tcpSourceTest.csv";
    remove(filePath.c_str());

    QueryServicePtr queryService = crd->getQueryService();
    QueryCatalogServicePtr queryCatalogService = crd->getQueryCatalogService();

    //register query
    auto query = Query::from("tcpStream").sink(FileSinkDescriptor::create(filePath));
    QueryId queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    Optimizer::PlacementStrategy::BottomUp,
                                                    FaultToleranceType::NONE,
                                                    LineageType::IN_MEMORY);
    EXPECT_NE(queryId, INVALID_QUERY_ID);
    auto globalQueryPlan = crd->getGlobalQueryPlan();
    ASSERT_TRUE(TestUtils::waitForQueryToStart(queryId, queryCatalogService));

    int connection;
    std::thread serverThread([&connection, this] {
        connection = sendMessages("{\"id\":\"42\", \"value\":\"5.893\", \"name\":\"hello\"}", 6);
    });
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(wrk1, queryId, globalQueryPlan, 2));
    ASSERT_TRUE(TestUtils::checkCompleteOrTimeout(crd, queryId, globalQueryPlan, 2));
    serverThread.join();

    x_INFO("QueryDeploymentTest: Remove query");
    queryService->validateAndQueueStopQueryRequest(queryId);
    ASSERT_TRUE(TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService));

    ASSERT_EQ(connection, 0);

    std::ifstream ifs(filePath.c_str());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    string expectedContent = "tcpStream$id:INTEGER(32 bits),tcpStream$value:Float(32 bits),tcpStream$name:ArrayType\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n"
                             "42,5.893000,hello\n";

    x_INFO("TCPSourceIntegrationTest: content={}", content);
    x_INFO("TCPSourceIntegrationTest: expContent={}", expectedContent);
    EXPECT_EQ(content, expectedContent);

    bool retStopWrk = wrk1->stop(false);
    ASSERT_TRUE(retStopWrk);

    bool retStopCord = crd->stopCoordinator(false);
    ASSERT_TRUE(retStopCord);
}

}// namespace x