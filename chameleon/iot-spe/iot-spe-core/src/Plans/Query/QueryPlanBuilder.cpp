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

#include <API/AttributeField.hpp>
#include <API/Query.hpp>
#include <API/WindowedQuery.hpp>
#include <Catalogs/UDF/UDFDescriptor.hpp>
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Nodes/Expressions/FieldRenameExpressionNode.hpp>
#include <Operators/LogicalOperators/LogicalBinaryOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/LogicalSourceDescriptor.hpp>
#include <Operators/LogicalOperators/WatermarkAssignerLogicalOperatorNode.hpp>
#include <Plans/Query/QueryPlanBuilder.hpp>
#include <Util/Common.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/Watermark/EventTimeWatermarkStrategyDescriptor.hpp>
#include <Windowing/Watermark/IngestionTimeWatermarkStrategyDescriptor.hpp>
#include <Windowing/WindowActions/CompleteAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/LazyxtLoopJoinTriggerActionDescriptor.hpp>
#include <Windowing/WindowPolicies/OnWatermarkChangeTriggerPolicyDescription.hpp>
#include <Windowing/WindowTypes/TimeBasedWindowType.hpp>
#include <iostream>
#include <utility>

namespace x {

QueryPlanPtr QueryPlanBuilder::createQueryPlan(std::string sourceName) {
    x_DEBUG("QueryPlanBuilder: create query plan for input source  {}", sourceName);
    auto sourceOperator = LogicalOperatorFactory::createSourceOperator(LogicalSourceDescriptor::create(sourceName));
    auto queryPlanPtr = QueryPlan::create(sourceOperator);
    queryPlanPtr->setSourceConsumed(sourceName);
    return queryPlanPtr;
}

QueryPlanPtr QueryPlanBuilder::addProjection(std::vector<x::ExpressionNodePtr> expressions, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add projection operator to query plan");
    OperatorNodePtr op = LogicalOperatorFactory::createProjectionOperator(expressions);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

QueryPlanPtr QueryPlanBuilder::addRename(std::string const& newSourceName, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add rename operator to query plan");
    auto op = LogicalOperatorFactory::createRenameSourceOperator(newSourceName);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

QueryPlanPtr QueryPlanBuilder::addFilter(x::ExpressionNodePtr const& filterExpression, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add filter operator to query plan");
    if (!filterExpression->getNodesByType<FieldRenameExpressionNode>().empty()) {
        x_THROW_RUNTIME_ERROR("QueryPlanBuilder: Filter predicate cannot have a FieldRenameExpression");
    }
    OperatorNodePtr op = LogicalOperatorFactory::createFilterOperator(filterExpression);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

QueryPlanPtr QueryPlanBuilder::addLimit(const uint64_t limit, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add limit operator to query plan");
    OperatorNodePtr op = LogicalOperatorFactory::createLimitOperator(limit);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::addMapUDF(Catalogs::UDF::UDFDescriptorPtr const& descriptor, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add map java udf operator to query plan");
    auto op = LogicalOperatorFactory::createMapUDFLogicalOperator(descriptor);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::addFlatMapUDF(Catalogs::UDF::UDFDescriptorPtr const& descriptor,
                                                  x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add flat map java udf operator to query plan");
    auto op = LogicalOperatorFactory::createFlatMapUDFLogicalOperator(descriptor);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

QueryPlanPtr QueryPlanBuilder::addMap(x::FieldAssignmentExpressionNodePtr const& mapExpression, x::QueryPlanPtr queryPlan) {
    x_DEBUG("QueryPlanBuilder: add map operator to query plan");
    if (!mapExpression->getNodesByType<FieldRenameExpressionNode>().empty()) {
        x_THROW_RUNTIME_ERROR("QueryPlanBuilder: Map expression cannot have a FieldRenameExpression");
    }
    OperatorNodePtr op = LogicalOperatorFactory::createMapOperator(mapExpression);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

QueryPlanPtr QueryPlanBuilder::addUnion(x::QueryPlanPtr leftQueryPlan, x::QueryPlanPtr rightQueryPlan) {
    x_DEBUG("QueryPlanBuilder: unionWith the subQuery to current query plan");
    OperatorNodePtr op = LogicalOperatorFactory::createUnionOperator();
    leftQueryPlan = addBinaryOperatorAndUpdateSource(op, leftQueryPlan, rightQueryPlan);
    return leftQueryPlan;
}

QueryPlanPtr QueryPlanBuilder::addJoin(x::QueryPlanPtr leftQueryPlan,
                                       x::QueryPlanPtr rightQueryPlan,
                                       ExpressionItem onLeftKey,
                                       ExpressionItem onRightKey,
                                       const Windowing::WindowTypePtr& windowType,
                                       Join::LogicalJoinDefinition::JoinType joinType) {
    x_DEBUG("Query: joinWith the subQuery to current query");

    auto leftKeyFieldAccess = checkExpression(onLeftKey.getExpressionNode(), "leftSide");
    auto rightQueryPlanKeyFieldAccess = checkExpression(onRightKey.getExpressionNode(), "leftSide");

    //we use a on time trigger as default that triggers on each change of the watermark
    auto triggerPolicy = Windowing::OnWatermarkChangeTriggerPolicyDescription::create();
    //    auto triggerPolicy = OnTimeTriggerPolicyDescription::create(1000);

    //we use a lazy NL join because this is currently the only one that is implemented
    auto triggerAction = Join::LazyxtLoopJoinTriggerActionDescriptor::create();

    // we use a complete window type as we currently do not have a distributed join
    auto distrType = Windowing::DistributionCharacteristic::createCompleteWindowType();

    x_ASSERT(rightQueryPlan && !rightQueryPlan->getRootOperators().empty(), "invalid rightQueryPlan query plan");
    auto rootOperatorRhs = rightQueryPlan->getRootOperators()[0];
    auto leftJoinType = leftQueryPlan->getRootOperators()[0]->getOutputSchema();
    auto rightQueryPlanJoinType = rootOperatorRhs->getOutputSchema();

    // check if query contain watermark assigner, and add if missing (as default behaviour)
    leftQueryPlan = checkAndAddWatermarkAssignment(leftQueryPlan, windowType);
    rightQueryPlan = checkAndAddWatermarkAssignment(rightQueryPlan, windowType);

    //TODO 1,1 should be replaced once we have distributed joins with the number of child input edges
    //TODO(Ventura?>Steffen) can we know this at this query submission time?
    auto joinDefinition = Join::LogicalJoinDefinition::create(leftKeyFieldAccess,
                                                              rightQueryPlanKeyFieldAccess,
                                                              windowType,
                                                              distrType,
                                                              triggerPolicy,
                                                              triggerAction,
                                                              1,
                                                              1,
                                                              joinType);

    x_DEBUG("QueryPlanBuilder: add join operator to query plan");
    auto op = LogicalOperatorFactory::createJoinOperator(joinDefinition);
    leftQueryPlan = addBinaryOperatorAndUpdateSource(op, leftQueryPlan, rightQueryPlan);
    return leftQueryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::addBatchJoin(x::QueryPlanPtr leftQueryPlan,
                                                 x::QueryPlanPtr rightQueryPlan,
                                                 ExpressionItem onProbeKey,
                                                 ExpressionItem onBuildKey) {
    x_DEBUG("Query: joinWith the subQuery to current query");
    auto probeKeyFieldAccess = checkExpression(onProbeKey.getExpressionNode(), "onProbeKey");
    auto buildKeyFieldAccess = checkExpression(onBuildKey.getExpressionNode(), "onBuildKey");

    x_ASSERT(rightQueryPlan && !rightQueryPlan->getRootOperators().empty(), "invalid rightQueryPlan query plan");
    auto rootOperatorRhs = rightQueryPlan->getRootOperators()[0];
    auto leftJoinType = leftQueryPlan->getRootOperators()[0]->getOutputSchema();
    auto rightQueryPlanJoinType = rootOperatorRhs->getOutputSchema();

    // todo here again we wan't to extend to distributed joins:
    //TODO 1,1 should be replaced once we have distributed joins with the number of child input edges
    //TODO(Ventura?>Steffen) can we know this at this query submission time?
    auto joinDefinition = Join::Experimental::LogicalBatchJoinDefinition::create(buildKeyFieldAccess, probeKeyFieldAccess, 1, 1);

    auto op = LogicalOperatorFactory::createBatchJoinOperator(joinDefinition);
    leftQueryPlan = addBinaryOperatorAndUpdateSource(op, leftQueryPlan, rightQueryPlan);
    return leftQueryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::addSink(x::QueryPlanPtr queryPlan, x::SinkDescriptorPtr sinkDescriptor) {
    OperatorNodePtr op = LogicalOperatorFactory::createSinkOperator(sinkDescriptor);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

x::QueryPlanPtr
QueryPlanBuilder::assignWatermark(x::QueryPlanPtr queryPlan,
                                  x::Windowing::WatermarkStrategyDescriptorPtr const& watermarkStrategyDescriptor) {
    OperatorNodePtr op = LogicalOperatorFactory::createWatermarkAssignerOperator(watermarkStrategyDescriptor);
    queryPlan->appendOperatorAsNewRoot(op);
    return queryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::checkAndAddWatermarkAssignment(x::QueryPlanPtr queryPlan,
                                                                   const x::Windowing::WindowTypePtr windowType) {
    x_DEBUG("QueryPlanBuilder: checkAndAddWatermarkAssignment for a (sub)query plan");
    auto timeBasedWindowType = Windowing::WindowType::asTimeBasedWindowType(windowType);

    if (queryPlan->getOperatorByType<WatermarkAssignerLogicalOperatorNode>().empty()) {
        if (timeBasedWindowType->getTimeCharacteristic()->getType() == Windowing::TimeCharacteristic::Type::IngestionTime) {
            return assignWatermark(queryPlan, Windowing::IngestionTimeWatermarkStrategyDescriptor::create());
        } else if (timeBasedWindowType->getTimeCharacteristic()->getType() == Windowing::TimeCharacteristic::Type::EventTime) {
            return assignWatermark(queryPlan,
                                   Windowing::EventTimeWatermarkStrategyDescriptor::create(
                                       Attribute(timeBasedWindowType->getTimeCharacteristic()->getField()->getName()),
                                       API::Milliseconds(0),
                                       timeBasedWindowType->getTimeCharacteristic()->getTimeUnit()));
        }
    }
    return queryPlan;
}

x::QueryPlanPtr QueryPlanBuilder::addBinaryOperatorAndUpdateSource(x::OperatorNodePtr operatorNode,
                                                                     x::QueryPlanPtr leftQueryPlan,
                                                                     x::QueryPlanPtr rightQueryPlan) {
    leftQueryPlan->addRootOperator(rightQueryPlan->getRootOperators()[0]);
    leftQueryPlan->appendOperatorAsNewRoot(operatorNode);
    x_DEBUG("QueryPlanBuilder: addBinaryOperatorAndUpdateSource: update the source names");
    auto newSourceName = x::Util::updateSourceName(leftQueryPlan->getSourceConsumed(), rightQueryPlan->getSourceConsumed());
    leftQueryPlan->setSourceConsumed(newSourceName);
    return leftQueryPlan;
}

std::shared_ptr<FieldAccessExpressionNode> QueryPlanBuilder::checkExpression(x::ExpressionNodePtr expression,
                                                                             std::string side) {
    if (!expression->instanceOf<FieldAccessExpressionNode>()) {
        x_ERROR("Query: window key ({}) has to be an FieldAccessExpression but it was a  {}", side, expression->toString());
        x_THROW_RUNTIME_ERROR("Query: window key has to be an FieldAccessExpression");
    }
    return expression->as<FieldAccessExpressionNode>();
}
}// namespace x