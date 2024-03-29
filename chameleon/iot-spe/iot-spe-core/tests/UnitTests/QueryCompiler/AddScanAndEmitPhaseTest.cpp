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

#include <API/Expressions/Expressions.hpp>
#include <API/QueryAPI.hpp>
#include <BaseIntegrationTest.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Nodes/Expressions/ConstantValueExpressionNode.hpp>
#include <Operators/LogicalOperators/LogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/PrintSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/LogicalSourceDescriptor.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/Operators/OperatorPipeline.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalEmitOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalFilterOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalScanOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalSinkOperator.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalSourceOperator.hpp>
#include <QueryCompiler/Operators/PipelineQueryPlan.hpp>
#include <QueryCompiler/Phases/AddScanAndEmitPhase.hpp>
#include <Sources/DefaultSource.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

namespace x {

using namespace x::API;
using namespace x::QueryCompilation::PhysicalOperators;

class AddScanAndEmitPhaseTest : public Testing::BaseUnitTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("AddScanAndEmitPhase.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup AddScanAndEmitPhase test class.");
    }
};

/**
 * @brief Input Query Plan:
 * Input:
 * | Physical Source |
 *
 * Expected Result:
 * | Physical Source |
 *
 */
TEST_F(AddScanAndEmitPhaseTest, scanOperator) {
    auto pipelineQueryPlan = QueryCompilation::PipelineQueryPlan::create();
    auto operatorPlan = QueryCompilation::OperatorPipeline::createSourcePipeline();

    auto source =
        QueryCompilation::PhysicalOperators::PhysicalSourceOperator::create(SchemaPtr(), SchemaPtr(), SourceDescriptorPtr());
    operatorPlan->prependOperator(source);
    pipelineQueryPlan->addPipeline(operatorPlan);

    auto phase = QueryCompilation::AddScanAndEmitPhase::create();
    pipelineQueryPlan = phase->apply(pipelineQueryPlan);

    auto pipelineRootOperator = pipelineQueryPlan->getSourcePipelix()[0]->getQueryPlan()->getRootOperators()[0];

    ASSERT_INSTANCE_OF(pipelineRootOperator, PhysicalSourceOperator);
    ASSERT_EQ(pipelineRootOperator->getChildren().size(), 0U);
}

/**
 * @brief Input Query Plan:
 * Input:
 * | Physical Sink |
 *
 * Expected Result:
 * | Physical Sink |
 *
 */
TEST_F(AddScanAndEmitPhaseTest, sinkOperator) {
    auto operatorPlan = QueryCompilation::OperatorPipeline::create();
    auto sink = QueryCompilation::PhysicalOperators::PhysicalSinkOperator::create(SchemaPtr(), SchemaPtr(), SinkDescriptorPtr());
    operatorPlan->prependOperator(sink);

    auto phase = QueryCompilation::AddScanAndEmitPhase::create();
    operatorPlan = phase->process(operatorPlan);

    auto pipelineRootOperator = operatorPlan->getQueryPlan()->getRootOperators()[0];

    ASSERT_INSTANCE_OF(pipelineRootOperator, PhysicalSinkOperator);
    ASSERT_EQ(pipelineRootOperator->getChildren().size(), 0U);
}

/**
 * @brief Input Query Plan:
 * Input:
 * | Physical Filter |
 *
 * Expected Result:
 * | Physical Scan --- Physical Filter --- Physical Emit|
 *
 */
TEST_F(AddScanAndEmitPhaseTest, pipelineFilterQuery) {

    auto operatorPlan = QueryCompilation::OperatorPipeline::create();
    operatorPlan->prependOperator(PhysicalFilterOperator::create(SchemaPtr(), SchemaPtr(), ExpressionNodePtr()));

    auto phase = QueryCompilation::AddScanAndEmitPhase::create();
    operatorPlan = phase->process(operatorPlan);

    auto pipelineRootOperator = operatorPlan->getQueryPlan()->getRootOperators()[0];

    ASSERT_INSTANCE_OF(pipelineRootOperator, PhysicalScanOperator);
    auto filter = pipelineRootOperator->getChildren()[0];
    ASSERT_INSTANCE_OF(filter, PhysicalFilterOperator);
    auto emit = filter->getChildren()[0];
    ASSERT_INSTANCE_OF(emit, PhysicalEmitOperator);
}
}// namespace x