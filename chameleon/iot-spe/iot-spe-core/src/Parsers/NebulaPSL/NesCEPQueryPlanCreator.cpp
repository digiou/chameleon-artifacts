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
#include <API/QueryAPI.hpp>
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/AndExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/EqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/GreaterEqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/GreaterExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/LessEqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/OrExpressionNode.hpp>
#include <Operators/LogicalOperators/LogicalBinaryOperatorNode.hpp>
#include <Operators/LogicalOperators/WatermarkAssignerLogicalOperatorNode.hpp>
#include <Parsers/IoTSPEPSL/IoTSPEPSLQueryPlanCreator.hpp>
#include <Plans/Query/QueryPlanBuilder.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/WindowActions/CompleteAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/LazyxtLoopJoinTriggerActionDescriptor.hpp>
#include <Windowing/WindowPolicies/OnWatermarkChangeTriggerPolicyDescription.hpp>

namespace x::Parsers {

void xCEPQueryPlanCreator::enterListEvents(xCEPParser::ListEventsContext* context) {
    x_DEBUG("xCEPQueryPlanCreator : enterListEvents: init tree walk and initialize read out of AST ");
    this->nodeId++;
    xCEPBaseListener::enterListEvents(context);
}

void xCEPQueryPlanCreator::enterEventElem(xCEPParser::EventElemContext* cxt) {
    x_DEBUG("xCEPQueryPlanCreator : exitEventElem: found a stream source  {}", cxt->getStart()->getText());
    //create sources pair, e.g., <identifier,SourceName>
    pattern.addSource(std::make_pair(sourceCounter, cxt->getStart()->getText()));
    this->lastSeenSourcePtr = sourceCounter;
    sourceCounter++;
    x_DEBUG("xCEPQueryPlanCreator : exitEventElem: inserted {} to sources", cxt->getStart()->getText());
    this->nodeId++;
    xCEPBaseListener::exitEventElem(cxt);
}

void xCEPQueryPlanCreator::exitOperatorRule(xCEPParser::OperatorRuleContext* context) {
    x_DEBUG("xCEPQueryPlanCreator : exitOperatorRule: create a node for the operator  {}", context->getText());
    //create Operator node and set attributes with context information
    IoTSPEPSLOperatorNode node = IoTSPEPSLOperatorNode(nodeId);
    node.setParentNodeId(-1);
    node.setOperatorName(context->getText());
    node.setLeftChildId(lastSeenSourcePtr);
    node.setRightChildId(lastSeenSourcePtr + 1);
    pattern.addOperatorNode(node);
    // increase nodeId
    nodeId++;
    xCEPBaseListener::exitOperatorRule(context);
}

void xCEPQueryPlanCreator::exitInputStream(xCEPParser::InputStreamContext* context) {
    x_DEBUG("xCEPQueryPlanCreator : exitInputStream: replace alias with streamName  {}", context->getText());
    std::string sourceName = context->getStart()->getText();
    std::string aliasName = context->getStop()->getText();
    //replace alias in the list of sources with actual sources name
    std::map<int32_t, std::string> sources = pattern.getSources();
    std::map<int32_t, std::string>::iterator sourceMapItem;
    for (sourceMapItem = sources.begin(); sourceMapItem != sources.end(); sourceMapItem++) {
        std::string currentEventName = sourceMapItem->second;
        if (currentEventName == aliasName) {
            pattern.updateSource(sourceMapItem->first, sourceName);
            break;
        }
    }
    xCEPBaseListener::exitInputStream(context);
}

void xCEPQueryPlanCreator::enterWhereExp(xCEPParser::WhereExpContext* context) {
    inWhere = true;
    xCEPBaseListener::enterWhereExp(context);
}

void xCEPQueryPlanCreator::exitWhereExp(xCEPParser::WhereExpContext* context) {
    inWhere = false;
    xCEPBaseListener::exitWhereExp(context);
}

// WITHIN clause
void xCEPQueryPlanCreator::exitInterval(xCEPParser::IntervalContext* cxt) {
    x_DEBUG("xCEPQueryPlanCreator : exitInterval:  {}", cxt->getText());
    // get window definitions
    std::string timeUnit = cxt->intervalType()->getText();
    int32_t time = std::stoi(cxt->getStart()->getText());
    pattern.setWindow(std::make_pair(timeUnit, time));
    xCEPBaseListener::exitInterval(cxt);
}

void xCEPQueryPlanCreator::enterOutAttribute(xCEPParser::OutAttributeContext* context) {
    //get projection fields
    auto attributeField = x::Attribute(context->NAME()->getText()).getExpressionNode();
    pattern.addProjectionField(attributeField);
}

void xCEPQueryPlanCreator::enterSink(xCEPParser::SinkContext* context) {
    // collect specified sinks
    std::string sinkType = context->sinkType()->getText();
    SinkDescriptorPtr sinkDescriptor;

    if (sinkType == "Print") {
        sinkDescriptor = x::PrintSinkDescriptor::create();
    }
    if (sinkType == "File") {
        sinkDescriptor = x::FileSinkDescriptor::create(context->NAME()->getText());
    }
    if (sinkType == "MQTT") {
        sinkDescriptor = x::NullOutputSinkDescriptor::create();
    }
    if (sinkType == "Network") {
        sinkDescriptor = x::NullOutputSinkDescriptor::create();
    }
    if (sinkType == "NullOutput") {
        sinkDescriptor = x::NullOutputSinkDescriptor::create();
    }
    pattern.addSink(sinkDescriptor);
    xCEPBaseListener::enterSink(context);
}

void xCEPQueryPlanCreator::exitSinkList(xCEPParser::SinkListContext* context) { xCEPBaseListener::exitSinkList(context); }

void xCEPQueryPlanCreator::enterQuantifiers(xCEPParser::QuantifiersContext* context) {
    x_DEBUG("xCEPQueryPlanCreator : enterQuantifiers: {}", context->getText())
    //method that specifies the times operator which has several cases
    //create Operator node and add specification
    IoTSPEPSLOperatorNode timeOperatorNode = IoTSPEPSLOperatorNode(nodeId);
    timeOperatorNode.setParentNodeId(-1);
    timeOperatorNode.setOperatorName("TIMES");
    timeOperatorNode.setLeftChildId(lastSeenSourcePtr);
    if (context->LBRACKET()) {    // context contains []
        if (context->D_POINTS()) {//e.g., A[2:10] means that we expect at least 2 and maximal 10 occurrences of A
            x_DEBUG("xCEPQueryPlanCreator : enterQuantifiers: Times with Min: {} and Max {}",
                      context->iterMin()->INT()->getText(),
                      context->iterMin()->INT()->getText());
            timeOperatorNode.setMinMax(
                std::make_pair(stoi(context->iterMin()->INT()->getText()), stoi(context->iterMax()->INT()->getText())));
        } else {// e.g., A[2] means that we except exact 2 occurrences of A
            timeOperatorNode.setMinMax(std::make_pair(stoi(context->INT()->getText()), stoi(context->INT()->getText())));
        }
    } else if (context->PLUS()) {//e.g., A+, means a occurs at least once
        timeOperatorNode.setMinMax(std::make_pair(0, 0));
    } else if (context->STAR()) {//[]*   //TODO unbounded iteration variant not yet implemented #866
        x_THROW_RUNTIME_ERROR(
            "xCEPQueryPlanCreator : enterQuantifiers: x currently does not support the iteration variant *");
    }
    pattern.addOperatorNode(timeOperatorNode);
    //update pointer
    nodeId++;
    xCEPBaseListener::enterQuantifiers(context);
}

void xCEPQueryPlanCreator::exitBinaryComparisonPredicate(xCEPParser::BinaryComparasionPredicateContext* context) {
    //retrieve the ExpressionNode for the filter and save it in the pattern expressionList
    std::string comparisonOperator = context->comparisonOperator()->getText();
    auto leftExpressionNode = x::Attribute(this->currentLeftExp).getExpressionNode();
    auto rightExpressionNode = x::Attribute(this->currentRightExp).getExpressionNode();
    x::ExpressionNodePtr expression;
    x_DEBUG("xCEPQueryPlanCreator: exitBinaryComparisonPredicate: add filters {} {} {}",
              this->currentLeftExp,
              comparisonOperator,
              this->currentRightExp)

    if (comparisonOperator == "<") {
        expression = x::LessExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == "<=") {
        expression = x::LessEqualsExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == ">") {
        expression = x::GreaterExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == ">=") {
        expression = x::GreaterEqualsExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == "==") {
        expression = x::EqualsExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == "&&") {
        expression = x::AndExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    if (comparisonOperator == "||") {
        expression = x::OrExpressionNode::create(leftExpressionNode, rightExpressionNode);
    }
    this->pattern.addExpression(expression);
}

void xCEPQueryPlanCreator::enterAttribute(xCEPParser::AttributeContext* cxt) {
    x_DEBUG("xCEPQueryPlanCreator: enterAttribute: {}", cxt->getText())
    if (inWhere) {
        if (leftFilter) {
            currentLeftExp = cxt->getText();
            leftFilter = false;
        } else {
            currentRightExp = cxt->getText();
            leftFilter = true;
        }
    }
}

QueryPlanPtr xCEPQueryPlanCreator::createQueryFromPatternList() const {
    if (this->pattern.getOperatorList().empty() && this->pattern.getSources().size() == 0) {
        x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreator: createQueryFromPatternList: Received an empty pattern");
    }
    x_DEBUG("xCEPQueryPlanCreator: createQueryFromPatternList: create query from AST elements")
    QueryPlanPtr queryPlan;
    // if for simple patterns without binary CEP operators
    if (this->pattern.getOperatorList().empty() && this->pattern.getSources().size() == 1) {
        auto sourceName = pattern.getSources().at(0);
        queryPlan = QueryPlanBuilder::createQueryPlan(sourceName);
        // else for pattern with binary operators
    } else {
        // iterate over OperatorList, create and add LogicalOperatorNodes
        for (auto operatorNode = pattern.getOperatorList().begin(); operatorNode != pattern.getOperatorList().end();
             ++operatorNode) {
            auto operatorName = operatorNode->second.getOperatorName();
            // add binary operators
            if (operatorName == "OR" || operatorName == "SEQ" || operatorName == "AND") {
                queryPlan = addBinaryOperatorToQueryPlan(operatorName, operatorNode, queryPlan);
            } else if (operatorName == "TIMES") {//add times operator
                x_DEBUG("xCEPQueryPlanCreater: createQueryFromPatternList: add unary operator {}", operatorName)
                auto sourceName = pattern.getSources().at(operatorNode->second.getLeftChildId());
                queryPlan = QueryPlanBuilder::createQueryPlan(sourceName);
                x_DEBUG("xCEPQueryPlanCreater: createQueryFromPatternList: add times operator{}",
                          pattern.getSources().at(operatorNode->second.getLeftChildId()))

                queryPlan = QueryPlanBuilder::addMap(Attribute("Count") = 1, queryPlan);

                //create window
                auto timeMeasurements = transformWindowToTimeMeasurements(pattern.getWindow().first, pattern.getWindow().second);
                if (timeMeasurements.first.getTime() != TimeMeasure(0).getTime()) {
                    auto windowType = Windowing::SlidingWindow::of(EventTime(Attribute("timestamp")),
                                                                   timeMeasurements.first,
                                                                   timeMeasurements.second);
                    // check and add watermark
                    queryPlan = QueryPlanBuilder::checkAndAddWatermarkAssignment(queryPlan, windowType);
                    // create default pol
                    auto triggerPolicy = Windowing::OnWatermarkChangeTriggerPolicyDescription::create();
                    auto distributionType = Windowing::DistributionCharacteristic::createCompleteWindowType();
                    auto triggerAction = Windowing::CompleteAggregationTriggerActionDescriptor::create();

                    std::vector<WindowAggregationPtr> windowAggs;
                    std::shared_ptr<WindowAggregationDescriptor> sumAgg = API::Sum(Attribute("Count"));

                    auto timeField =
                        WindowType::asTimeBasedWindowType(windowType)->getTimeCharacteristic()->getField()->getName();
                    std::shared_ptr<WindowAggregationDescriptor> maxAggForTime = API::Max(Attribute(timeField));
                    windowAggs.push_back(sumAgg);
                    windowAggs.push_back(maxAggForTime);

                    auto windowDefinition = Windowing::LogicalWindowDefinition::create(windowAggs,
                                                                                       windowType,
                                                                                       distributionType,
                                                                                       triggerPolicy,
                                                                                       triggerAction,
                                                                                       0);

                    OperatorNodePtr op = LogicalOperatorFactory::createWindowOperator(windowDefinition);
                    queryPlan->appendOperatorAsNewRoot(op);

                    int32_t min = operatorNode->second.getMinMax().first;
                    int32_t max = operatorNode->second.getMinMax().second;

                    if (min != 0 && max != 0) {//TODO unbounded iteration variant not yet implemented #866 (min = 1/0 max = 0)
                        ExpressionNodePtr predicate;

                        if (min == max) {
                            predicate = Attribute("Count") = min;
                        } else if (min == 0) {
                            predicate = Attribute("Count") <= max;
                        } else if (max == 0) {
                            predicate = Attribute("Count") >= min;
                        } else {
                            predicate = Attribute("Count") >= min && Attribute("Count") <= max;
                        }
                        queryPlan = QueryPlanBuilder::addFilter(predicate, queryPlan);
                    }
                } else {
                    x_THROW_RUNTIME_ERROR(
                        "xCEPQueryPlanCreator: createQueryFromPatternList: The Iteration operator requires a time window.");
                }
            } else {
                x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreator: createQueryFromPatternList: Unkown CEP operator"
                                        << operatorName);
            }
        }
    }
    if (!pattern.getExpressions().empty()) {
        queryPlan = addFilters(queryPlan);
    }
    if (!pattern.getProjectionFields().empty()) {
        queryPlan = addProjections(queryPlan);
    }

    const std::vector<x::OperatorNodePtr>& rootOperators = queryPlan->getRootOperators();
    // add the sinks to the query plan
    for (SinkDescriptorPtr sinkDescriptor : pattern.getSinks()) {
        auto sinkOperator = LogicalOperatorFactory::createSinkOperator(sinkDescriptor);
        for (auto& rootOperator : rootOperators) {
            sinkOperator->addChild(rootOperator);
            queryPlan->removeAsRootOperator(rootOperator);
            queryPlan->addRootOperator(sinkOperator);
        }
    }
    return queryPlan;
}

QueryPlanPtr xCEPQueryPlanCreator::addFilters(QueryPlanPtr queryPlan) const {
    for (auto it = pattern.getExpressions().begin(); it != pattern.getExpressions().end(); ++it) {
        queryPlan = QueryPlanBuilder::addFilter(*it, queryPlan);
    }
    return queryPlan;
}

std::pair<TimeMeasure, TimeMeasure> xCEPQueryPlanCreator::transformWindowToTimeMeasurements(std::string timeMeasure,
                                                                                              int32_t timeValue) const {

    if (timeMeasure == "MILLISECOND") {
        TimeMeasure size = Minutes(timeValue);
        TimeMeasure slide = Minutes(1);
        return std::pair<TimeMeasure, TimeMeasure>(size, slide);
    } else if (timeMeasure == "SECOND") {
        TimeMeasure size = Seconds(timeValue);
        TimeMeasure slide = Seconds(1);
        return std::pair<TimeMeasure, TimeMeasure>(size, slide);
    } else if (timeMeasure == "MINUTE") {
        TimeMeasure size = Minutes(timeValue);
        TimeMeasure slide = Minutes(1);
        return std::pair<TimeMeasure, TimeMeasure>(size, slide);
    } else if (timeMeasure == "HOUR") {
        TimeMeasure size = Hours(timeValue);
        TimeMeasure slide = Hours(1);
        return std::pair<TimeMeasure, TimeMeasure>(size, slide);
    } else {
        x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreator: transformWindowToTimeMeasurements: Unkown time measure " + timeMeasure);
    }
    return std::pair<TimeMeasure, TimeMeasure>(TimeMeasure(0), TimeMeasure(0));
}

QueryPlanPtr xCEPQueryPlanCreator::addProjections(QueryPlanPtr queryPlan) const {
    return QueryPlanBuilder::addProjection(pattern.getProjectionFields(), queryPlan);
}

QueryPlanPtr xCEPQueryPlanCreator::getQueryPlan() const {
    try {
        return createQueryFromPatternList();
    } catch (std::exception& e) {
        x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreator::getQueryPlan(): Was not able to parse query: " << e.what());
    }
}

std::string xCEPQueryPlanCreator::keyAssignment(std::string keyName) const {
    //first, get unique ids for the key attributes
    auto cepRightId = Util::getNextOperatorId();
    //second, create a unique name for both key attributes
    std::string cepRightKey = keyName + std::to_string(cepRightId);
    return cepRightKey;
}

QueryPlanPtr xCEPQueryPlanCreator::addBinaryOperatorToQueryPlan(std::string operaterName,
                                                                  std::map<int, IoTSPEPSLOperatorNode>::const_iterator it,
                                                                  QueryPlanPtr queryPlan) const {
    QueryPlanPtr rightQueryPlan;
    QueryPlanPtr leftQueryPlan;
    x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: add binary operator {}", operaterName)
    // find left (query) and right branch (subquery) of binary operator
    //left query plan
    x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: create subqueryLeft from {}",
              pattern.getSources().at(it->second.getLeftChildId()))
    auto leftSourceName = pattern.getSources().at(it->second.getLeftChildId());
    auto rightSourceName = pattern.getSources().at(it->second.getRightChildId());
    // if queryPlan is empty
    if (!queryPlan
        || (queryPlan->getSourceConsumed().find(leftSourceName) == std::string::npos
            && queryPlan->getSourceConsumed().find(rightSourceName) == std::string::npos)) {
        x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: both sources are not in the current queryPlan")
        //make left source current queryPlan
        leftQueryPlan = QueryPlanBuilder::createQueryPlan(leftSourceName);
        //right source as right queryPlan
        rightQueryPlan = QueryPlanBuilder::createQueryPlan(rightSourceName);
    } else {
        x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: check for the right branch")
        leftQueryPlan = queryPlan;
        rightQueryPlan = checkIfSourceIsAlreadyConsumedSource(leftSourceName, rightSourceName, queryPlan);
    }

    if (operaterName == "OR") {
        x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: addUnionOperator")
        leftQueryPlan = QueryPlanBuilder::addUnion(leftQueryPlan, rightQueryPlan);
    } else {
        // Seq and And require a window, so first we create and check the window operator
        auto timeMeasurements = transformWindowToTimeMeasurements(pattern.getWindow().first, pattern.getWindow().second);
        if (timeMeasurements.first.getTime() != TimeMeasure(0).getTime()) {
            auto windowType =
                Windowing::SlidingWindow::of(EventTime(Attribute("timestamp")), timeMeasurements.first, timeMeasurements.second);

            //Next, we add artificial key attributes to the sources in order to reuse the join-logic later
            std::string cepLeftKey = keyAssignment("cep_leftLeft");
            std::string cepRightKey = keyAssignment("cep_rightkey");

            //next: add Map operator that maps the attributes with value 1 to the left and right source
            leftQueryPlan = QueryPlanBuilder::addMap(Attribute(cepLeftKey) = 1, leftQueryPlan);
            rightQueryPlan = QueryPlanBuilder::addMap(Attribute(cepRightKey) = 1, rightQueryPlan);

            //then, define the artificial attributes as key attributes
            x_DEBUG("xCEPQueryPlanCreater: add name cepLeftKey {} and name cepRightKey {}", cepLeftKey, cepRightKey);
            ExpressionItem onLeftKey = ExpressionItem(Attribute(cepLeftKey)).getExpressionNode();
            ExpressionItem onRightKey = ExpressionItem(Attribute(cepRightKey)).getExpressionNode();
            auto leftKeyFieldAccess = onLeftKey.getExpressionNode()->as<FieldAccessExpressionNode>();
            auto rightKeyFieldAccess = onRightKey.getExpressionNode()->as<FieldAccessExpressionNode>();

            leftQueryPlan = QueryPlanBuilder::addJoin(leftQueryPlan,
                                                      rightQueryPlan,
                                                      onLeftKey,
                                                      onRightKey,
                                                      windowType,
                                                      Join::LogicalJoinDefinition::JoinType::CARTESIAN_PRODUCT);

            if (operaterName == "SEQ") {
                // for SEQ we need to add additional filter for order by time
                auto timestamp = WindowType::asTimeBasedWindowType(windowType)->getTimeCharacteristic()->getField()->getName();
                // to guarantee a correct order of events by time (sequence) we need to identify the correct source and its timestamp
                // in case of composed streams on the right branch
                auto sourceNameRight = rightQueryPlan->getSourceConsumed();
                if (sourceNameRight.find("_") != std::string::npos) {
                    // we find the most left source and use its timestamp for the filter constraint
                    uint64_t posStart = sourceNameRight.find("_");
                    uint64_t posEnd = sourceNameRight.find("_", posStart + 1);
                    sourceNameRight = sourceNameRight.substr(posStart + 1, posEnd - 2) + "$" + timestamp;
                }// in case the right branch only contains 1 source we can just use it
                else {
                    sourceNameRight = sourceNameRight + "$" + timestamp;
                }
                auto sourceNameLeft = leftQueryPlan->getSourceConsumed();
                // in case of composed sources on the left branch
                if (sourceNameLeft.find("_") != std::string::npos) {
                    // we find the most right source and use its timestamp for the filter constraint
                    uint64_t posStart = sourceNameLeft.find_last_of("_");
                    sourceNameLeft = sourceNameLeft.substr(posStart + 1) + "$" + timestamp;
                }// in case the left branch only contains 1 source we can just use it
                else {
                    sourceNameLeft = sourceNameLeft + "$" + timestamp;
                }
                x_DEBUG("xCEPQueryPlanCreater: ExpressionItem for Left Source {} and ExpressionItem for Right Source {}",
                          sourceNameLeft,
                          sourceNameRight);
                //create filter expression and add it to queryPlan
                leftQueryPlan =
                    QueryPlanBuilder::addFilter(Attribute(sourceNameLeft) < Attribute(sourceNameRight), leftQueryPlan);
            }
        } else {
            x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreater: createQueryFromPatternList: Cannot create " + operaterName
                                    + "without a window.");
        }
    }
    return leftQueryPlan;
}

QueryPlanPtr xCEPQueryPlanCreator::checkIfSourceIsAlreadyConsumedSource(std::basic_string<char> leftSourceName,
                                                                          std::basic_string<char> rightSourceName,
                                                                          QueryPlanPtr queryPlan) const {
    QueryPlanPtr rightQueryPlan = nullptr;
    if (queryPlan->getSourceConsumed().find(leftSourceName) != std::string::npos
        && queryPlan->getSourceConsumed().find(rightSourceName) != std::string::npos) {
        x_THROW_RUNTIME_ERROR("xCEPQueryPlanCreater: checkIfSourceIsAlreadyConsumedSource: Both sources are already consumed "
                                "and combined with a binary operator");
    } else if (queryPlan->getSourceConsumed().find(leftSourceName) == std::string::npos) {
        // right queryplan
        x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: create subqueryRight from {}", leftSourceName)
        return rightQueryPlan = QueryPlanBuilder::createQueryPlan(leftSourceName);
    } else if (queryPlan->getSourceConsumed().find(rightSourceName) == std::string::npos) {
        // right queryplan
        x_DEBUG("xCEPQueryPlanCreater: addBinaryOperatorToQueryPlan: create subqueryRight from {}", rightSourceName)
        return rightQueryPlan = QueryPlanBuilder::createQueryPlan(rightSourceName);
    }
    return rightQueryPlan;
}

}// namespace x::Parsers