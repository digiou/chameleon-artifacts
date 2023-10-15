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
#include <API/Schema.hpp>
#include <BaseIntegrationTest.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestExecutionEngine.hpp>
#include <Util/TestSinkDescriptor.hpp>
#include <Util/TestSourceDescriptor.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <Windowing/WindowTypes/ThresholdWindow.hpp>
#include <iostream>
#include <utility>

using namespace x;
using Runtime::TupleBuffer;

// Dump IR
constexpr auto dumpMode = x::QueryCompilation::QueryCompilerOptions::DumpMode::NONE;

class NonKeyedTumblingWindowQueryExecutionTest
    : public Testing::BaseUnitTest,
      public ::testing::WithParamInterface<QueryCompilation::QueryCompilerOptions::QueryCompiler> {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("NonKeyedTumblingWindowQueryExecutionTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("QueryExecutionTest: Setup NonKeyedTumblingWindowQueryExecutionTest test class.");
    }
    /* Will be called before a test is executed. */
    void SetUp() override {
        Testing::BaseUnitTest::SetUp();
        auto queryCompiler = this->GetParam();
        executionEngine = std::make_shared<Testing::TestExecutionEngine>(queryCompiler, dumpMode);
    }

    /* Will be called before a test is executed. */
    void TearDown() override {
        x_DEBUG("QueryExecutionTest: Tear down NonKeyedTumblingWindowQueryExecutionTest test case.");
        ASSERT_TRUE(executionEngine->stop());
        Testing::BaseUnitTest::TearDown();
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() {
        x_DEBUG("QueryExecutionTest: Tear down NonKeyedTumblingWindowQueryExecutionTest test class.");
    }

    std::shared_ptr<Testing::TestExecutionEngine> executionEngine;
};

void fillBuffer(Runtime::MemoryLayouts::DynamicTupleBuffer& buf) {
    for (int recordIndex = 0; recordIndex < 9; recordIndex++) {
        buf[recordIndex][0].write<uint64_t>(recordIndex);
        buf[recordIndex][1].write<int64_t>(recordIndex * 10);
    }
    // close the window
    buf[9][0].write<uint64_t>(0);
    buf[9][1].write<int64_t>(0);
    buf.setNumberOfTuples(10);
}

TEST_P(NonKeyedTumblingWindowQueryExecutionTest, testTumblingWindow) {
    auto sourceSchema = Schema::create()->addField("test$f1", BasicType::UINT64)->addField("test$f2", BasicType::INT64);
    auto testSourceDescriptor = executionEngine->createDataSource(sourceSchema);

    auto sinkSchema = Schema::create()->addField("test$sum", BasicType::INT64);
    auto testSink = executionEngine->createDataSink(sinkSchema);

    auto testSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(testSink);
    auto query = TestQuery::from(testSourceDescriptor)
                     .window(TumblingWindow::of(EventTime(Attribute("test$f1")), Milliseconds(5)))
                     .apply(Sum(Attribute("test$f2", BasicType::INT64))->as(Attribute("test$sum")))
                     .project(Attribute("test$sum"))
                     .sink(testSinkDescriptor);

    auto plan = executionEngine->submitQuery(query.getQueryPlan());

    auto source = executionEngine->getDataSource(plan, 0);
    auto inputBuffer = executionEngine->getBuffer(sourceSchema);
    fillBuffer(inputBuffer);
    ASSERT_EQ(inputBuffer.getBuffer().getNumberOfTuples(), 10);
    source->emitBuffer(inputBuffer);
    testSink->waitTillCompleted();

    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 1u);
    auto resultBuffer = testSink->getResultBuffer(0);

    EXPECT_EQ(resultBuffer.getNumberOfTuples(), 1u);
    EXPECT_EQ(resultBuffer[0][0].read<int64_t>(), 100LL);// sum

    ASSERT_TRUE(executionEngine->stopQuery(plan));
    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 0U);
}

TEST_P(NonKeyedTumblingWindowQueryExecutionTest, testSimpleTumblingWindowNoProjection) {
    auto sourceSchema = Schema::create()->addField("test$f1", BasicType::UINT64)->addField("test$f2", BasicType::INT64);
    auto testSourceDescriptor = executionEngine->createDataSource(sourceSchema);

    auto sinkSchema = Schema::create()
                          ->addField("test$start", BasicType::INT64)
                          ->addField("test$end", BasicType::INT64)
                          ->addField("test$sum", BasicType::INT64);
    auto testSink = executionEngine->createDataSink(sinkSchema);

    auto testSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(testSink);
    auto query = TestQuery::from(testSourceDescriptor)
                     .window(TumblingWindow::of(EventTime(Attribute("test$f1")), Milliseconds(5)))
                     .apply(Sum(Attribute("test$f2", BasicType::INT64))->as(Attribute("test$sum")))
                     .sink(testSinkDescriptor);

    auto plan = executionEngine->submitQuery(query.getQueryPlan());

    auto source = executionEngine->getDataSource(plan, 0);
    auto inputBuffer = executionEngine->getBuffer(sourceSchema);
    fillBuffer(inputBuffer);
    ASSERT_EQ(inputBuffer.getBuffer().getNumberOfTuples(), 10);
    source->emitBuffer(inputBuffer);
    testSink->waitTillCompleted();

    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 1u);
    auto resultBuffer = testSink->getResultBuffer(0);

    EXPECT_EQ(resultBuffer.getNumberOfTuples(), 1u);
    EXPECT_EQ(resultBuffer[0][0].read<int64_t>(), 0LL);  // start
    EXPECT_EQ(resultBuffer[0][1].read<int64_t>(), 5LL);  // end
    EXPECT_EQ(resultBuffer[0][2].read<int64_t>(), 100LL);// sum

    ASSERT_TRUE(executionEngine->stopQuery(plan));
    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 0U);
}

INSTANTIATE_TEST_CASE_P(testGlobalTumblingWindow,
                        NonKeyedTumblingWindowQueryExecutionTest,
                        ::testing::Values(QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER),
                        [](const testing::TestParamInfo<NonKeyedTumblingWindowQueryExecutionTest::ParamType>& info) {
                            return std::string(magic_enum::enum_name(info.param));
                        });