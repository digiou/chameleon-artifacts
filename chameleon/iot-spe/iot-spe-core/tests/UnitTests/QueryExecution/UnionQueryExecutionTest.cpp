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
#include "API/Expressions/Expressions.hpp"
#include <Util/TestExecutionEngine.hpp>
#include <Util/TestSinkDescriptor.hpp>

using namespace x;

constexpr auto dumpMode = x::QueryCompilation::QueryCompilerOptions::DumpMode::NONE;

class UnionQueryExecutionTest : public Testing::BaseUnitTest,
                                public ::testing::WithParamInterface<QueryCompilation::QueryCompilerOptions::QueryCompiler> {
  public:
    struct __attribute__((packed)) DefaultRecord {
        int64_t id;
        int64_t value;
    };

    static void SetUpTestCase() {
        x::Logger::setupLogging("FilterQueryExecutionTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("FilterQueryExecutionTest: Setup FilterQueryExecutionTest test class.");
    }
    /* Will be called before a test is executed. */
    void SetUp() override {
        Testing::BaseUnitTest::SetUp();
        auto queryCompiler = this->GetParam();
        executionEngine = std::make_shared<Testing::TestExecutionEngine>(queryCompiler, dumpMode);

        // Setup default parameters.
        defaultSchema = Schema::create()->addField("test$id", BasicType::INT64)->addField("test$one", BasicType::INT64);
        defaultDataGenerator = [](Runtime::MemoryLayouts::DynamicTupleBuffer& buffer, uint64_t numInputRecords) {
            for (size_t recordIdx = 0; recordIdx < numInputRecords; ++recordIdx) {
                buffer[recordIdx][0].write<int64_t>(recordIdx);
                buffer[recordIdx][1].write<int64_t>(1);
            }
            buffer.setNumberOfTuples(numInputRecords);
        };
        defaultSource = executionEngine->createDataSource(defaultSchema);
        defaultSink = executionEngine->createCollectSink<DefaultRecord>(defaultSchema);
        defaultTestSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(defaultSink);
    }

    /* Will be called before a test is executed. */
    void TearDown() override {
        x_DEBUG("FilterQueryExecutionTest: Tear down FilterQueryExecutionTest test case.");
        ASSERT_TRUE(executionEngine->stop());
        Testing::BaseUnitTest::TearDown();
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_DEBUG("FilterQueryExecutionTest: Tear down FilterQueryExecutionTest test class."); }

    void generateAndEmitInputBuffers(
        const std::shared_ptr<Runtime::Execution::ExecutableQueryPlan>& queryPlan,
        const std::vector<SchemaPtr>& sourceSchemas,
        std::vector<std::function<void(Runtime::MemoryLayouts::DynamicTupleBuffer&, uint64_t)>> inputDataGenerators,
        uint64_t numInputTuples = 10) {
        // Make sure that each source schema has one corresponding input data generator.
        EXPECT_EQ(sourceSchemas.size(), inputDataGenerators.size());

        // For each source schema, create a source and an input buffer. Fill the input buffer using the corresponding
        // input data generator and finally use the source to emit the input buffer.
        for (size_t sourceSchemaIdx = 0; sourceSchemaIdx < sourceSchemas.size(); ++sourceSchemaIdx) {
            auto source = executionEngine->getDataSource(queryPlan, sourceSchemaIdx);
            auto inputBuffer = executionEngine->getBuffer(sourceSchemas.at(sourceSchemaIdx));

            inputDataGenerators.at(sourceSchemaIdx)(inputBuffer, numInputTuples);

            source->emitBuffer(inputBuffer);
        }
    }

    SchemaPtr defaultSchema;
    std::shared_ptr<SourceDescriptor> defaultSource;
    std::function<void(Runtime::MemoryLayouts::DynamicTupleBuffer&, uint64_t numInputRecords)> defaultDataGenerator;
    std::shared_ptr<CollectTestSink<DefaultRecord>> defaultSink;
    std::shared_ptr<x::TestUtils::TestSinkDescriptor> defaultTestSinkDescriptor;
    std::shared_ptr<Testing::TestExecutionEngine> executionEngine;
    static constexpr uint64_t millisecondsToHours = 3600000;
    static constexpr uint64_t defaultTimeoutInMilliseconds = 5000;
};

TEST_P(UnionQueryExecutionTest, unionOperatorWithFilterOnUnionResult) {
    // Setup test parameters.
    constexpr uint64_t numInputRecords = 10;
    constexpr uint64_t numResultRecords = 8;

    // Define query plan.
    Query query = TestQuery::from(defaultSource)
                      .unionWith(TestQuery::from(defaultSource))
                      .filter(Attribute("test$id") > 5)
                      .sink(defaultTestSinkDescriptor);
    auto queryPlan = executionEngine->submitQuery(query.getQueryPlan());

    // Generate input and run query.
    generateAndEmitInputBuffers(queryPlan,
                                {defaultSchema, defaultSchema},
                                {defaultDataGenerator, defaultDataGenerator},
                                numInputRecords);
    defaultSink->waitTillCompletedOrTimeout(numResultRecords, defaultTimeoutInMilliseconds);
    const auto resultRecords = defaultSink->getResult();

    EXPECT_EQ(resultRecords.size(), numResultRecords);
    for (size_t recordIdx = 0; recordIdx < resultRecords.size(); ++recordIdx) {
        EXPECT_EQ(resultRecords.at(recordIdx).id, (recordIdx % 4) + 6);
        EXPECT_EQ(resultRecords.at(recordIdx).value, 1);
    }

    ASSERT_TRUE(executionEngine->stopQuery(queryPlan));
}

TEST_P(UnionQueryExecutionTest, unionOperatorWithFilterOnSources) {
    // Setup test parameters.
    constexpr uint64_t numInputRecords = 10;
    constexpr uint64_t numResultRecords = 8;

    // Define query plan.
    Query subQuery = TestQuery::from(defaultSource).filter(Attribute("test$id") > 3);
    Query query =
        TestQuery::from(defaultSource).filter(Attribute("test$id") > 7).unionWith(subQuery).sink(defaultTestSinkDescriptor);
    auto queryPlan = executionEngine->submitQuery(query.getQueryPlan());

    // Generate input and run query.
    generateAndEmitInputBuffers(queryPlan,
                                {defaultSchema, defaultSchema},
                                {defaultDataGenerator, defaultDataGenerator},
                                numInputRecords);
    defaultSink->waitTillCompletedOrTimeout(numResultRecords, defaultTimeoutInMilliseconds);
    const auto resultRecords = defaultSink->getResult();

    EXPECT_EQ(resultRecords.size(), numResultRecords);
    for (size_t recordIdx = 0; recordIdx < resultRecords.size(); ++recordIdx) {
        EXPECT_EQ(resultRecords.at(recordIdx).id, (recordIdx + 4) % 6 + 4);//result ids: 8,9,4,5,6,7,8,9
        EXPECT_EQ(resultRecords.at(recordIdx).value, 1);
    }

    ASSERT_TRUE(executionEngine->stopQuery(queryPlan));
}

TEST_P(UnionQueryExecutionTest, unionOperatorWithoutExecution) {
    // Setup test parameters.
    constexpr uint64_t numInputRecords = 10;
    constexpr uint64_t numResultRecords = 20;

    // Define query plan.
    Query query = TestQuery::from(defaultSource).unionWith(TestQuery::from(defaultSource)).sink(defaultTestSinkDescriptor);
    auto queryPlan = executionEngine->submitQuery(query.getQueryPlan());

    // Generate input and run query.
    generateAndEmitInputBuffers(queryPlan,
                                {defaultSchema, defaultSchema},
                                {defaultDataGenerator, defaultDataGenerator},
                                numInputRecords);
    defaultSink->waitTillCompletedOrTimeout(numResultRecords, defaultTimeoutInMilliseconds);
    const auto resultRecords = defaultSink->getResult();

    EXPECT_EQ(resultRecords.size(), numResultRecords);
    for (size_t recordIdx = 0; recordIdx < resultRecords.size(); ++recordIdx) {
        EXPECT_EQ(resultRecords.at(recordIdx).id, recordIdx % 10);
        EXPECT_EQ(resultRecords.at(recordIdx).value, 1);
    }

    ASSERT_TRUE(executionEngine->stopQuery(queryPlan));
}

TEST_P(UnionQueryExecutionTest, unionOperatorWithoutDifferentSchemasAndManualProject) {
    // Setup test parameters.
    constexpr uint64_t numInputRecords = 10;
    constexpr uint64_t numResultRecords = 20;

    auto customSchema = Schema::create()->addField("custom$id", BasicType::INT64)->addField("custom$one", BasicType::INT64);
    auto customSource = executionEngine->createDataSource(customSchema);

    // Define query plan.
    Query query = TestQuery::from(defaultSource)
                      .unionWith(TestQuery::from(customSource)
                                     .project(Attribute("custom$id").as("test$id"), Attribute("custom$one").as("test$one")))
                      .sink(defaultTestSinkDescriptor);
    auto queryPlan = executionEngine->submitQuery(query.getQueryPlan());

    // Generate input and run query.
    generateAndEmitInputBuffers(queryPlan,
                                {defaultSchema, customSchema},
                                {defaultDataGenerator, defaultDataGenerator},
                                numInputRecords);
    defaultSink->waitTillCompletedOrTimeout(numResultRecords, defaultTimeoutInMilliseconds);
    const auto resultRecords = defaultSink->getResult();

    EXPECT_EQ(resultRecords.size(), numResultRecords);
    for (size_t recordIdx = 0; recordIdx < resultRecords.size(); ++recordIdx) {
        EXPECT_EQ(resultRecords.at(recordIdx).id, recordIdx % 10);
        EXPECT_EQ(resultRecords.at(recordIdx).value, 1);
    }

    ASSERT_TRUE(executionEngine->stopQuery(queryPlan));
}

TEST_P(UnionQueryExecutionTest, unionOperatorWithoutResults) {
    // Setup test parameters.
    constexpr uint64_t numInputRecords = 10;
    constexpr uint64_t numResultRecords = 0;

    // Define query plan.
    Query query = TestQuery::from(defaultSource)
                      .unionWith(TestQuery::from(defaultSource))
                      .filter(Attribute("test$id") > 9)
                      .sink(defaultTestSinkDescriptor);

    auto queryPlan = executionEngine->submitQuery(query.getQueryPlan());

    // Generate input and run query.
    generateAndEmitInputBuffers(queryPlan,
                                {defaultSchema, defaultSchema},
                                {defaultDataGenerator, defaultDataGenerator},
                                numInputRecords);
    defaultSink->waitTillCompleted(numResultRecords);
    const auto resultRecords = defaultSink->getResult();

    EXPECT_EQ(resultRecords.size(), numResultRecords);
    ASSERT_TRUE(executionEngine->stopQuery(queryPlan));
}

INSTANTIATE_TEST_CASE_P(testFilterQueries,
                        UnionQueryExecutionTest,
                        ::testing::Values(QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER),
                        [](const testing::TestParamInfo<UnionQueryExecutionTest::ParamType>& info) {
                            return std::string(magic_enum::enum_name(info.param));
                        });
