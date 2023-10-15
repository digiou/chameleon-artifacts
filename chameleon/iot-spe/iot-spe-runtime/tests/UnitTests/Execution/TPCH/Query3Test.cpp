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

#include <API/Schema.hpp>
#include <BaseIntegrationTest.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Execution/Aggregation/AvgAggregation.hpp>
#include <Execution/Aggregation/CountAggregation.hpp>
#include <Execution/Aggregation/MaxAggregation.hpp>
#include <Execution/Aggregation/MinAggregation.hpp>
#include <Execution/Aggregation/SumAggregation.hpp>
#include <Execution/Expressions/ArithmeticalExpressions/AddExpression.hpp>
#include <Execution/Expressions/ArithmeticalExpressions/MulExpression.hpp>
#include <Execution/Expressions/ArithmeticalExpressions/SubExpression.hpp>
#include <Execution/Expressions/ConstantValueExpression.hpp>
#include <Execution/Expressions/LogicalExpressions/AndExpression.hpp>
#include <Execution/Expressions/LogicalExpressions/EqualsExpression.hpp>
#include <Execution/Expressions/LogicalExpressions/GreaterThanExpression.hpp>
#include <Execution/Expressions/LogicalExpressions/LessThanExpression.hpp>
#include <Execution/Expressions/ReadFieldExpression.hpp>
#include <Execution/Expressions/WriteFieldExpression.hpp>
#include <Execution/MemoryProvider/ColumnMemoryProvider.hpp>
#include <Execution/MemoryProvider/RowMemoryProvider.hpp>
#include <Execution/Operators/Emit.hpp>
#include <Execution/Operators/Relational/Aggregation/BatchKeyedAggregation.hpp>
#include <Execution/Operators/Relational/Aggregation/BatchKeyedAggregationHandler.hpp>
#include <Execution/Operators/Relational/Join/BatchJoinBuild.hpp>
#include <Execution/Operators/Relational/Join/BatchJoinHandler.hpp>
#include <Execution/Operators/Relational/Join/BatchJoinProbe.hpp>
#include <Execution/Operators/Relational/Map.hpp>
#include <Execution/Operators/Relational/Selection.hpp>
#include <Execution/Operators/Scan.hpp>
#include <Execution/Pipelix/CompilationPipelineProvider.hpp>
#include <Execution/Pipelix/PhysicalOperatorPipeline.hpp>
#include <Execution/RecordBuffer.hpp>
#include <Nautilus/Interface/Hash/MurMur3HashFunction.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/WorkerContext.hpp>
#include <TPCH/Query3.hpp>
#include <TPCH/TPCHTableGenerator.hpp>
#include <TestUtils/AbstractPipelineExecutionTest.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/Timer.hpp>
#include <gtest/gtest.h>
#include <memory>

namespace x::Runtime::Execution {
using namespace Expressions;
using namespace Operators;
class TPCH_Q3 : public Testing::BaseUnitTest, public AbstractPipelineExecutionTest {

  public:
    TPCH_Scale_Factor targetScaleFactor = TPCH_Scale_Factor::F0_01;
    Nautilus::CompilationOptions options;
    ExecutablePipelineProvider* provider;
    std::shared_ptr<Runtime::BufferManager> bm;
    std::shared_ptr<Runtime::BufferManager> table_bm;
    std::shared_ptr<WorkerContext> wc;
    std::unordered_map<TPCHTable, std::unique_ptr<x::Runtime::Table>> tables;

    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("TPCH_Q3.log", x::LogLevel::LOG_DEBUG);

        x_INFO("Setup TPCH_Q3 test class.");
    }

    /* Will be called before a test is executed. */
    void SetUp() override {
        Testing::BaseUnitTest::SetUp();
        x_INFO("Setup TPCH_Q3 test case.");
        if (!ExecutablePipelineProviderRegistry::hasPlugin(GetParam())) {
            GTEST_SKIP();
        }
        provider = ExecutablePipelineProviderRegistry::getPlugin(this->GetParam()).get();
        table_bm = std::make_shared<Runtime::BufferManager>(8 * 1024 * 1024, 1000);
        bm = std::make_shared<Runtime::BufferManager>();
        wc = std::make_shared<WorkerContext>(0, bm, 100);
        tables = TPCHTableGenerator(table_bm, targetScaleFactor).generate();
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_INFO("Tear down TPCH_Q6 test class."); }
};

TEST_P(TPCH_Q3, joinPipeline) {
    auto& customers = tables[TPCHTable::Customer];
    auto& orders = tables[TPCHTable::Orders];
    auto& lineitems = tables[TPCHTable::LineItem];

    auto plan = TPCH_Query3::getPipelinePlan(tables, bm);

    // process query
    auto pipeline1 = plan.getPipeline(0);
    auto pipeline2 = plan.getPipeline(1);
    auto pipeline3 = plan.getPipeline(2);
    auto aggExecutablePipeline = provider->create(pipeline1.pipeline, options);
    auto orderCustomersJoinBuildPipeline = provider->create(pipeline2.pipeline, options);
    auto lineitems_ordersJoinBuildPipeline = provider->create(pipeline3.pipeline, options);

    aggExecutablePipeline->setup(*pipeline1.ctx);
    orderCustomersJoinBuildPipeline->setup(*pipeline2.ctx);
    lineitems_ordersJoinBuildPipeline->setup(*pipeline3.ctx);

    for (auto& chunk : customers->getChunks()) {
        aggExecutablePipeline->execute(chunk, *pipeline1.ctx, *wc);
    }

    auto joinHandler = pipeline1.ctx->getOperatorHandler<BatchJoinHandler>(0);
    auto numberOfKeys = joinHandler->getThreadLocalState(wc->getId())->getNumberOfEntries();
    EXPECT_EQ(numberOfKeys, 337);
    auto hm = joinHandler->mergeState();
    EXPECT_EQ(hm->getCurrentSize(), 337);

    for (auto& chunk : orders->getChunks()) {
        orderCustomersJoinBuildPipeline->execute(chunk, *pipeline2.ctx, *wc);
    }
    auto joinHandler2 = pipeline2.ctx->getOperatorHandler<BatchJoinHandler>(1);
    auto numberOfKeys2 = joinHandler2->getThreadLocalState(wc->getId())->getNumberOfEntries();
    EXPECT_EQ(numberOfKeys2, 1797);
    auto hm2 = joinHandler2->mergeState();
    EXPECT_EQ(hm2->getCurrentSize(), 1797);

    for (auto& chunk : lineitems->getChunks()) {
        lineitems_ordersJoinBuildPipeline->execute(chunk, *pipeline3.ctx, *wc);
    }
    auto aggHandler = pipeline3.ctx->getOperatorHandler<BatchKeyedAggregationHandler>(1);
    EXPECT_EQ(aggHandler->getThreadLocalStore(0)->getCurrentSize(), 138);

    aggExecutablePipeline->stop(*pipeline1.ctx);
    orderCustomersJoinBuildPipeline->stop(*pipeline2.ctx);
    lineitems_ordersJoinBuildPipeline->stop(*pipeline3.ctx);
}

INSTANTIATE_TEST_CASE_P(testIfCompilation,
                        TPCH_Q3,
                        ::testing::Values("BCInterpreter", "PipelineInterpreter", "PipelineCompiler"),
                        [](const testing::TestParamInfo<TPCH_Q3::ParamType>& info) {
                            return info.param;
                        });

}// namespace x::Runtime::Execution