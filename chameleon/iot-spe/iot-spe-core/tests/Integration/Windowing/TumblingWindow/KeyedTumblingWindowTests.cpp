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
#include <Catalogs/Source/PhysicalSourceTypes/LambdaSourceType.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Services/QueryService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>
#include <iostream>

using namespace std;
namespace x {
using namespace Configurations;

/**
 * @brief In this test we assess the correctxs of the thread local tumbling window
 */
class KeyedTumblingWindowTests : public Testing::BaseIntegrationTest, public ::testing::WithParamInterface<int> {
  public:
    WorkerConfigurationPtr workerConfiguration;
    static void SetUpTestCase() {
        x::Logger::setupLogging("KeyedTumblingWindowTests.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup KeyedTumblingWindowTests test class.");
    }

    void SetUp() override {
        Testing::BaseIntegrationTest::SetUp();
        workerConfiguration = WorkerConfiguration::create();
        workerConfiguration->queryCompiler.windowingStrategy = QueryCompilation::QueryCompilerOptions::WindowingStrategy::SLICING;
        workerConfiguration->queryCompiler.compilationStrategy =
            QueryCompilation::QueryCompilerOptions::CompilationStrategy::OPTIMIZE;
    }
};

struct InputValue {
    uint64_t value;
    uint64_t id;
    uint64_t timestamp;
};

struct __attribute__((__packed__)) Output {
    uint64_t start;
    uint64_t end;
    uint64_t id;
    uint64_t value;
    bool operator==(Output const& rhs) const {
        return (start == rhs.start && end == rhs.end && id == rhs.id && value == rhs.value);
    }
    friend ostream& operator<<(ostream& os, const Output& output) {
        os << "start: " << output.start << " end: " << output.end << " id: " << output.id << " value: " << output.value;
        return os;
    }
};

struct GlobalOutput {
    uint64_t start;
    uint64_t end;
    uint64_t value;
    bool operator==(GlobalOutput const& rhs) const { return (start == rhs.start && end == rhs.end && value == rhs.value); }
    friend ostream& operator<<(ostream& os, const GlobalOutput& output) {
        os << "start: " << output.start << " end: " << output.end << " value: " << output.value;
        return os;
    }
};

struct OutputMultiAgg {
    uint64_t start;
    uint64_t end;
    uint64_t id;
    uint64_t value1;
    uint64_t value2;
    uint64_t value3;
    uint64_t value4;
    uint64_t value5;
    bool operator==(const OutputMultiAgg& rhs) const {
        return start == rhs.start && end == rhs.end && id == rhs.id && value1 == rhs.value1 && value2 == rhs.value2
            && value3 == rhs.value3 && value4 == rhs.value4 && value5 == rhs.value5;
    }
    bool operator!=(const OutputMultiAgg& rhs) const { return !(rhs == *this); }

    friend ostream& operator<<(ostream& os, const OutputMultiAgg& agg) {
        os << "start: " << agg.start << " end: " << agg.end << " id: " << agg.id << " value1: " << agg.value1
           << " value2: " << agg.value2 << " value3: " << agg.value3 << " value4: " << agg.value4 << " value5: " << agg.value5;
        return os;
    }
};

struct InputValueMultiKeys {
    uint64_t value;
    uint64_t key1;
    uint64_t key2;
    uint64_t key3;
    uint64_t timestamp;
};

struct OutputMultiKeys {
    uint64_t start;
    uint64_t end;
    uint64_t key1;
    uint64_t key2;
    uint64_t key3;
    uint64_t value;
    bool operator==(const OutputMultiKeys& rhs) const {
        return start == rhs.start && end == rhs.end && key1 == rhs.key1 && key2 == rhs.key2 && key3 == rhs.key3
            && value == rhs.value;
    }
    bool operator!=(const OutputMultiKeys& rhs) const { return !(rhs == *this); }
    friend ostream& operator<<(ostream& os, const OutputMultiKeys& keys) {
        os << "start: " << keys.start << " end: " << keys.end << " key1: " << keys.key1 << " key2: " << keys.key2
           << " key3: " << keys.key3 << " value: " << keys.value;
        return os;
    }
};

PhysicalSourceTypePtr createSimpleInputStream(uint64_t numberOfBuffers, uint64_t numberOfKeys = 1) {
    return LambdaSourceType::create(
        [numberOfKeys](Runtime::TupleBuffer& buffer, uint64_t numberOfTuplesToProduce) {
            auto inputValue = (InputValue*) buffer.getBuffer();
            for (uint64_t i = 0; i < numberOfTuplesToProduce; i++) {
                inputValue[i].value = 1;
                inputValue[i].id = i % numberOfKeys;
                inputValue[i].timestamp = 1;
            }
            buffer.setNumberOfTuples(numberOfTuplesToProduce);
        },
        numberOfBuffers,
        0,
        GatheringMode::INTERVAL_MODE);
}

class DataGeneratorMultiKey {
  public:
    DataGeneratorMultiKey(uint64_t numberOfBuffers, uint64_t numberOfKeys = 1)
        : numberOfBuffers(numberOfBuffers), numberOfKeys(numberOfKeys){};
    PhysicalSourceTypePtr getSource() {
        return LambdaSourceType::create(
            [this](Runtime::TupleBuffer& buffer, uint64_t numberOfTuplesToProduce) {
                auto inputValue = (InputValueMultiKeys*) buffer.getBuffer();
                for (uint64_t i = 0; i < numberOfTuplesToProduce; i++) {
                    inputValue[i].value = 1;
                    inputValue[i].key1 = i % numberOfKeys;
                    inputValue[i].key2 = i % numberOfKeys;
                    inputValue[i].key3 = i % numberOfKeys;
                    inputValue[i].timestamp = (counter * numberOfTuplesToProduce) + i;
                }
                counter++;
            },
            numberOfBuffers,
            0,
            GatheringMode::INTERVAL_MODE);
    }

  private:
    uint64_t numberOfBuffers;
    uint64_t numberOfKeys;
    std::atomic_uint64_t counter = 0;
};

class DataGenerator {
  public:
    DataGenerator(uint64_t numberOfBuffers) : numberOfBuffers(numberOfBuffers){};
    PhysicalSourceTypePtr getSource() {
        return LambdaSourceType::create(
            [this](Runtime::TupleBuffer& buffer, uint64_t numberOfTuplesToProduce) {
                auto inputValue = (InputValue*) buffer.getBuffer();
                for (uint64_t i = 0; i < numberOfTuplesToProduce; i++) {
                    inputValue[i].value = 1;
                    inputValue[i].id = 1;
                    inputValue[i].timestamp = (counter * numberOfTuplesToProduce) + i;
                }
                counter++;
                x_DEBUG("Counter: {}", counter)
                buffer.setNumberOfTuples(numberOfTuplesToProduce);
            },
            numberOfBuffers,
            0,
            GatheringMode::INTERVAL_MODE);
    }

  private:
    uint64_t numberOfBuffers;
    std::atomic_uint64_t counter = 0;
};

template<typename Key = uint64_t>
struct __attribute__((__packed__)) KeyedWindowOutput {
    uint64_t start;
    uint64_t end;
    Key id;
    uint64_t value;

    bool operator==(KeyedWindowOutput<Key> const& rhs) const {
        return (start == rhs.start && end == rhs.end && id == rhs.id && value == rhs.value);
    }

    friend ostream& operator<<(ostream& os, const KeyedWindowOutput<Key>& output) {
        os << "start: " << output.start << " end: " << output.end << " id: " << output.id << " value: " << output.value;
        return os;
    }
};

TEST_F(KeyedTumblingWindowTests, testSimpleWindowEventTime) {

    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value")));

    CSVSourceTypePtr sourceConfig = CSVSourceType::create();
    sourceConfig->setFilePath(std::string(TEST_DATA_DIRECTORY) + "window.csv");
    sourceConfig->setGatheringInterval(0);
    sourceConfig->setNumberOfTuplesToProducePerBuffer(3);
    sourceConfig->setNumberOfBuffersToProduce(3);

    TestHarxs testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                                  .enableNautilus()
                                  .addLogicalSource("window", testSchema)
                                  .attachWorkerWithCSVSourceToCoordinator("window", sourceConfig)
                                  .validate()
                                  .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);

    std::vector<KeyedWindowOutput<>> expectedOutput = {{1000, 2000, 1, 1},
                                                       {2000, 3000, 1, 2},
                                                       {1000, 2000, 4, 1},
                                                       {2000, 3000, 11, 2},
                                                       {1000, 2000, 12, 1},
                                                       {2000, 3000, 16, 2},
                                                       {3000, 4000, 1, 6},
                                                       {3000, 4000, 11, 3}};

    auto actualOutput = testHarxs.getOutput<KeyedWindowOutput<>>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testSingleTumblingWindowSingleBuffer) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value")));

    auto lambdaSource = createSimpleInputStream(1);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", lambdaSource, workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);

    testHarxs.validate().setupTopology();

    std::vector<Output> expectedOutput = {{0, 1000, 0, 170}};

    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testSingleTumblingWindowMultiBuffer) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value")));
    auto lambdaSource = createSimpleInputStream(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", lambdaSource, workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 0, 17000}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testMultipleTumblingWindowMultiBuffer) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value")));
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 1, 1000},
                                          {1000, 2000, 1, 1000},
                                          {2000, 3000, 1, 1000},
                                          {3000, 4000, 1, 1000},
                                          {4000, 5000, 1, 1000},
                                          {5000, 6000, 1, 1000},
                                          {6000, 7000, 1, 1000},
                                          {7000, 8000, 1, 1000},
                                          {8000, 9000, 1, 1000},
                                          {9000, 10000, 1, 1000},
                                          {10000, 11000, 1, 1000},
                                          {11000, 12000, 1, 1000},
                                          {12000, 13000, 1, 1000},
                                          {13000, 14000, 1, 1000},
                                          {14000, 15000, 1, 1000},
                                          {15000, 16000, 1, 1000},
                                          {16000, 17000, 1, 1000}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testSingleTumblingWindowMultiBufferMultipleKeys) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value")));
    auto lambdaSource = createSimpleInputStream(100, 100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", lambdaSource, workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput;
    for (uint64_t k = 0; k < 70; k++) {
        expectedOutput.push_back({0, 1000, k, 200});
    }
    for (uint64_t k = 70; k < 100; k++) {
        expectedOutput.push_back({0, 1000, k, 100});
    }
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testTumblingWindowCount) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Count());
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 1, 1000},
                                          {1000, 2000, 1, 1000},
                                          {2000, 3000, 1, 1000},
                                          {3000, 4000, 1, 1000},
                                          {4000, 5000, 1, 1000},
                                          {5000, 6000, 1, 1000},
                                          {6000, 7000, 1, 1000},
                                          {7000, 8000, 1, 1000},
                                          {8000, 9000, 1, 1000},
                                          {9000, 10000, 1, 1000},
                                          {10000, 11000, 1, 1000},
                                          {11000, 12000, 1, 1000},
                                          {12000, 13000, 1, 1000},
                                          {13000, 14000, 1, 1000},
                                          {14000, 15000, 1, 1000},
                                          {15000, 16000, 1, 1000},
                                          {16000, 17000, 1, 1000}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testTumblingWindowMin) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Min(Attribute("value")));
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 1, 1},
                                          {1000, 2000, 1, 1},
                                          {2000, 3000, 1, 1},
                                          {3000, 4000, 1, 1},
                                          {4000, 5000, 1, 1},
                                          {5000, 6000, 1, 1},
                                          {6000, 7000, 1, 1},
                                          {7000, 8000, 1, 1},
                                          {8000, 9000, 1, 1},
                                          {9000, 10000, 1, 1},
                                          {10000, 11000, 1, 1},
                                          {11000, 12000, 1, 1},
                                          {12000, 13000, 1, 1},
                                          {13000, 14000, 1, 1},
                                          {14000, 15000, 1, 1},
                                          {15000, 16000, 1, 1},
                                          {16000, 17000, 1, 1}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/*
 * @brief Test if the min aggregation can be deployed
 */
TEST_F(KeyedTumblingWindowTests, testTumblingWindowMin2) {
    struct __attribute__((__packed__)) Car {
        uint64_t key;
        uint64_t value;
        uint64_t timestamp;
    };

    auto carSchema = Schema::create()
                         ->addField("key", DataTypeFactory::createUInt64())
                         ->addField("value", DataTypeFactory::createUInt64())
                         ->addField("timestamp", DataTypeFactory::createUInt64());

    auto queryWithWindowOperator = Query::from("car")
                                       .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                                       .byKey(Attribute("key"))
                                       .apply(Min(Attribute("value")));
    TestHarxs testHarxs = TestHarxs(queryWithWindowOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                                  .enableNautilus()
                                  .addLogicalSource("car", carSchema)
                                  .attachWorkerWithMemorySourceToCoordinator("car");

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);

    testHarxs.pushElement<Car>({1, 15, 1000}, 2);
    testHarxs.pushElement<Car>({1, 99, 1500}, 2);
    testHarxs.pushElement<Car>({1, 20, 2000}, 2);

    testHarxs.validate().setupTopology();

    std::vector<Output> expectedOutput = {{1000, 2000, 1, 15}, {2000, 3000, 1, 20}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testTumblingWindowMax) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Max(Attribute("value")));
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 1, 1},
                                          {1000, 2000, 1, 1},
                                          {2000, 3000, 1, 1},
                                          {3000, 4000, 1, 1},
                                          {4000, 5000, 1, 1},
                                          {5000, 6000, 1, 1},
                                          {6000, 7000, 1, 1},
                                          {7000, 8000, 1, 1},
                                          {8000, 9000, 1, 1},
                                          {9000, 10000, 1, 1},
                                          {10000, 11000, 1, 1},
                                          {11000, 12000, 1, 1},
                                          {12000, 13000, 1, 1},
                                          {13000, 14000, 1, 1},
                                          {14000, 15000, 1, 1},
                                          {15000, 16000, 1, 1},
                                          {16000, 17000, 1, 1}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testTumblingWindowAVG) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Avg(Attribute("value")));
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<Output> expectedOutput = {{0, 1000, 1, 1},
                                          {1000, 2000, 1, 1},
                                          {2000, 3000, 1, 1},
                                          {3000, 4000, 1, 1},
                                          {4000, 5000, 1, 1},
                                          {5000, 6000, 1, 1},
                                          {6000, 7000, 1, 1},
                                          {7000, 8000, 1, 1},
                                          {8000, 9000, 1, 1},
                                          {9000, 10000, 1, 1},
                                          {10000, 11000, 1, 1},
                                          {11000, 12000, 1, 1},
                                          {12000, 13000, 1, 1},
                                          {13000, 14000, 1, 1},
                                          {14000, 15000, 1, 1},
                                          {15000, 16000, 1, 1},
                                          {16000, 17000, 1, 1}};
    std::vector<Output> actualOutput = testHarxs.getOutput<Output>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testSingleMultiKeyTumblingWindow) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("key1", DataTypeFactory::createUInt64())
                          ->addField("key2", DataTypeFactory::createUInt64())
                          ->addField("key3", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValueMultiKeys), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("key1"), Attribute("key2"), Attribute("key3"))
                     .apply(Sum(Attribute("value")));
    auto dg = DataGeneratorMultiKey(1, 102);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<OutputMultiKeys> expectedOutput;
    for (uint64_t k = 0; k < 102; k++) {
        expectedOutput.push_back({0, 1000, k, k, k, 1});
    }
    std::vector<OutputMultiKeys> actualOutput = testHarxs.getOutput<OutputMultiKeys>(102, "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

TEST_F(KeyedTumblingWindowTests, testTumblingWindowMultiAggregate) {
    auto testSchema = Schema::create()
                          ->addField("value", DataTypeFactory::createUInt64())
                          ->addField("id", DataTypeFactory::createUInt64())
                          ->addField("timestamp", DataTypeFactory::createUInt64());

    ASSERT_EQ(sizeof(InputValue), testSchema->getSchemaSizeInBytes());
    auto query = Query::from("window")
                     .window(TumblingWindow::of(EventTime(Attribute("timestamp")), Seconds(1)))
                     .byKey(Attribute("id"))
                     .apply(Sum(Attribute("value"))->as(Attribute("sum_value")),
                            Count()->as(Attribute("count_value")),
                            Min(Attribute("value"))->as(Attribute("min_value")),
                            Max(Attribute("value"))->as(Attribute("max_value")),
                            Avg(Attribute("value"))->as(Attribute("avg_value")));
    auto dg = DataGenerator(100);
    auto testHarxs = TestHarxs(query, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .enableNautilus()
                           .addLogicalSource("window", testSchema)
                           .attachWorkerWithLambdaSourceToCoordinator("window", dg.getSource(), workerConfiguration);

    ASSERT_EQ(testHarxs.getWorkerCount(), 1UL);
    testHarxs.validate().setupTopology();
    std::vector<OutputMultiAgg> expectedOutput = {{0, 1000, 1, 1000, 1000, 1, 1, 1},
                                                  {1000, 2000, 1, 1000, 1000, 1, 1, 1},
                                                  {2000, 3000, 1, 1000, 1000, 1, 1, 1},
                                                  {3000, 4000, 1, 1000, 1000, 1, 1, 1},
                                                  {4000, 5000, 1, 1000, 1000, 1, 1, 1},
                                                  {5000, 6000, 1, 1000, 1000, 1, 1, 1},
                                                  {6000, 7000, 1, 1000, 1000, 1, 1, 1},
                                                  {7000, 8000, 1, 1000, 1000, 1, 1, 1},
                                                  {8000, 9000, 1, 1000, 1000, 1, 1, 1},
                                                  {9000, 10000, 1, 1000, 1000, 1, 1, 1},
                                                  {10000, 11000, 1, 1000, 1000, 1, 1, 1},
                                                  {11000, 12000, 1, 1000, 1000, 1, 1, 1},
                                                  {12000, 13000, 1, 1000, 1000, 1, 1, 1},
                                                  {13000, 14000, 1, 1000, 1000, 1, 1, 1},
                                                  {14000, 15000, 1, 1000, 1000, 1, 1, 1},
                                                  {15000, 16000, 1, 1000, 1000, 1, 1, 1},
                                                  {16000, 17000, 1, 1000, 1000, 1, 1, 1}};
    std::vector<OutputMultiAgg> actualOutput =
        testHarxs.getOutput<OutputMultiAgg>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");
    ASSERT_EQ(actualOutput.size(), expectedOutput.size());
    ASSERT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

}// namespace x