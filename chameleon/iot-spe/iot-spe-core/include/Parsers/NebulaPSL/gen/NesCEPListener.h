
// Generated from IoTDB/x-core/src/Parsers/IoTSPEPSL/gen/xCEP.g4 by ANTLR 4.9.2

#ifndef x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPLISTENER_H_
#define x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPLISTENER_H_

#include <Parsers/IoTSPEPSL/gen/xCEPParser.h>
#include <antlr4-runtime.h>

namespace x::Parsers {

/**
 * This interface defix an abstract listener for a parse tree produced by xCEPParser.
 */
class xCEPListener : public antlr4::tree::ParseTreeListener {
  public:
    virtual void enterQuery(xCEPParser::QueryContext* ctx) = 0;
    virtual void exitQuery(xCEPParser::QueryContext* ctx) = 0;

    virtual void enterCepPattern(xCEPParser::CepPatternContext* ctx) = 0;
    virtual void exitCepPattern(xCEPParser::CepPatternContext* ctx) = 0;

    virtual void enterInputStreams(xCEPParser::InputStreamsContext* ctx) = 0;
    virtual void exitInputStreams(xCEPParser::InputStreamsContext* ctx) = 0;

    virtual void enterInputStream(xCEPParser::InputStreamContext* ctx) = 0;
    virtual void exitInputStream(xCEPParser::InputStreamContext* ctx) = 0;

    virtual void enterCompositeEventExpressions(xCEPParser::CompositeEventExpressionsContext* ctx) = 0;
    virtual void exitCompositeEventExpressions(xCEPParser::CompositeEventExpressionsContext* ctx) = 0;

    virtual void enterWhereExp(xCEPParser::WhereExpContext* ctx) = 0;
    virtual void exitWhereExp(xCEPParser::WhereExpContext* ctx) = 0;

    virtual void enterTimeConstraints(xCEPParser::TimeConstraintsContext* ctx) = 0;
    virtual void exitTimeConstraints(xCEPParser::TimeConstraintsContext* ctx) = 0;

    virtual void enterInterval(xCEPParser::IntervalContext* ctx) = 0;
    virtual void exitInterval(xCEPParser::IntervalContext* ctx) = 0;

    virtual void enterIntervalType(xCEPParser::IntervalTypeContext* ctx) = 0;
    virtual void exitIntervalType(xCEPParser::IntervalTypeContext* ctx) = 0;

    virtual void enterOption(xCEPParser::OptionContext* ctx) = 0;
    virtual void exitOption(xCEPParser::OptionContext* ctx) = 0;

    virtual void enterOutputExpression(xCEPParser::OutputExpressionContext* ctx) = 0;
    virtual void exitOutputExpression(xCEPParser::OutputExpressionContext* ctx) = 0;

    virtual void enterOutAttribute(xCEPParser::OutAttributeContext* ctx) = 0;
    virtual void exitOutAttribute(xCEPParser::OutAttributeContext* ctx) = 0;

    virtual void enterSinkList(xCEPParser::SinkListContext* ctx) = 0;
    virtual void exitSinkList(xCEPParser::SinkListContext* ctx) = 0;

    virtual void enterSink(xCEPParser::SinkContext* ctx) = 0;
    virtual void exitSink(xCEPParser::SinkContext* ctx) = 0;

    virtual void enterListEvents(xCEPParser::ListEventsContext* ctx) = 0;
    virtual void exitListEvents(xCEPParser::ListEventsContext* ctx) = 0;

    virtual void enterEventElem(xCEPParser::EventElemContext* ctx) = 0;
    virtual void exitEventElem(xCEPParser::EventElemContext* ctx) = 0;

    virtual void enterEvent(xCEPParser::EventContext* ctx) = 0;
    virtual void exitEvent(xCEPParser::EventContext* ctx) = 0;

    virtual void enterQuantifiers(xCEPParser::QuantifiersContext* ctx) = 0;
    virtual void exitQuantifiers(xCEPParser::QuantifiersContext* ctx) = 0;

    virtual void enterIterMax(xCEPParser::IterMaxContext* ctx) = 0;
    virtual void exitIterMax(xCEPParser::IterMaxContext* ctx) = 0;

    virtual void enterIterMin(xCEPParser::IterMinContext* ctx) = 0;
    virtual void exitIterMin(xCEPParser::IterMinContext* ctx) = 0;

    virtual void enterConsecutiveOption(xCEPParser::ConsecutiveOptionContext* ctx) = 0;
    virtual void exitConsecutiveOption(xCEPParser::ConsecutiveOptionContext* ctx) = 0;

    virtual void enterOperatorRule(xCEPParser::OperatorRuleContext* ctx) = 0;
    virtual void exitOperatorRule(xCEPParser::OperatorRuleContext* ctx) = 0;

    virtual void enterSequence(xCEPParser::SequenceContext* ctx) = 0;
    virtual void exitSequence(xCEPParser::SequenceContext* ctx) = 0;

    virtual void enterContiguity(xCEPParser::ContiguityContext* ctx) = 0;
    virtual void exitContiguity(xCEPParser::ContiguityContext* ctx) = 0;

    virtual void enterSinkType(xCEPParser::SinkTypeContext* ctx) = 0;
    virtual void exitSinkType(xCEPParser::SinkTypeContext* ctx) = 0;

    virtual void enterNullNotnull(xCEPParser::NullNotnullContext* ctx) = 0;
    virtual void exitNullNotnull(xCEPParser::NullNotnullContext* ctx) = 0;

    virtual void enterConstant(xCEPParser::ConstantContext* ctx) = 0;
    virtual void exitConstant(xCEPParser::ConstantContext* ctx) = 0;

    virtual void enterExpressions(xCEPParser::ExpressionsContext* ctx) = 0;
    virtual void exitExpressions(xCEPParser::ExpressionsContext* ctx) = 0;

    virtual void enterIsExpression(xCEPParser::IsExpressionContext* ctx) = 0;
    virtual void exitIsExpression(xCEPParser::IsExpressionContext* ctx) = 0;

    virtual void enterNotExpression(xCEPParser::NotExpressionContext* ctx) = 0;
    virtual void exitNotExpression(xCEPParser::NotExpressionContext* ctx) = 0;

    virtual void enterLogicalExpression(xCEPParser::LogicalExpressionContext* ctx) = 0;
    virtual void exitLogicalExpression(xCEPParser::LogicalExpressionContext* ctx) = 0;

    virtual void enterPredicateExpression(xCEPParser::PredicateExpressionContext* ctx) = 0;
    virtual void exitPredicateExpression(xCEPParser::PredicateExpressionContext* ctx) = 0;

    virtual void enterExpressionAtomPredicate(xCEPParser::ExpressionAtomPredicateContext* ctx) = 0;
    virtual void exitExpressionAtomPredicate(xCEPParser::ExpressionAtomPredicateContext* ctx) = 0;

    virtual void enterInPredicate(xCEPParser::InPredicateContext* ctx) = 0;
    virtual void exitInPredicate(xCEPParser::InPredicateContext* ctx) = 0;

    virtual void enterBinaryComparisonPredicate(xCEPParser::BinaryComparasionPredicateContext* ctx) = 0;
    virtual void exitBinaryComparisonPredicate(xCEPParser::BinaryComparasionPredicateContext* ctx) = 0;

    virtual void enterIsNullPredicate(xCEPParser::IsNullPredicateContext* ctx) = 0;
    virtual void exitIsNullPredicate(xCEPParser::IsNullPredicateContext* ctx) = 0;

    virtual void enterUnaryExpressionAtom(xCEPParser::UnaryExpressionAtomContext* ctx) = 0;
    virtual void exitUnaryExpressionAtom(xCEPParser::UnaryExpressionAtomContext* ctx) = 0;

    virtual void enterAttributeAtom(xCEPParser::AttributeAtomContext* ctx) = 0;
    virtual void exitAttributeAtom(xCEPParser::AttributeAtomContext* ctx) = 0;

    virtual void enterConstantExpressionAtom(xCEPParser::ConstantExpressionAtomContext* ctx) = 0;
    virtual void exitConstantExpressionAtom(xCEPParser::ConstantExpressionAtomContext* ctx) = 0;

    virtual void enterBinaryExpressionAtom(xCEPParser::BinaryExpressionAtomContext* ctx) = 0;
    virtual void exitBinaryExpressionAtom(xCEPParser::BinaryExpressionAtomContext* ctx) = 0;

    virtual void enterBitExpressionAtom(xCEPParser::BitExpressionAtomContext* ctx) = 0;
    virtual void exitBitExpressionAtom(xCEPParser::BitExpressionAtomContext* ctx) = 0;

    virtual void enterxtedExpressionAtom(xCEPParser::xtedExpressionAtomContext* ctx) = 0;
    virtual void exitxtedExpressionAtom(xCEPParser::xtedExpressionAtomContext* ctx) = 0;

    virtual void enterMathExpressionAtom(xCEPParser::MathExpressionAtomContext* ctx) = 0;
    virtual void exitMathExpressionAtom(xCEPParser::MathExpressionAtomContext* ctx) = 0;

    virtual void enterEventAttribute(xCEPParser::EventAttributeContext* ctx) = 0;
    virtual void exitEventAttribute(xCEPParser::EventAttributeContext* ctx) = 0;

    virtual void enterEventIteration(xCEPParser::EventIterationContext* ctx) = 0;
    virtual void exitEventIteration(xCEPParser::EventIterationContext* ctx) = 0;

    virtual void enterMathExpression(xCEPParser::MathExpressionContext* ctx) = 0;
    virtual void exitMathExpression(xCEPParser::MathExpressionContext* ctx) = 0;

    virtual void enterAggregation(xCEPParser::AggregationContext* ctx) = 0;
    virtual void exitAggregation(xCEPParser::AggregationContext* ctx) = 0;

    virtual void enterAttribute(xCEPParser::AttributeContext* ctx) = 0;
    virtual void exitAttribute(xCEPParser::AttributeContext* ctx) = 0;

    virtual void enterAttVal(xCEPParser::AttValContext* ctx) = 0;
    virtual void exitAttVal(xCEPParser::AttValContext* ctx) = 0;

    virtual void enterBoolRule(xCEPParser::BoolRuleContext* ctx) = 0;
    virtual void exitBoolRule(xCEPParser::BoolRuleContext* ctx) = 0;

    virtual void enterCondition(xCEPParser::ConditionContext* ctx) = 0;
    virtual void exitCondition(xCEPParser::ConditionContext* ctx) = 0;

    virtual void enterUnaryOperator(xCEPParser::UnaryOperatorContext* ctx) = 0;
    virtual void exitUnaryOperator(xCEPParser::UnaryOperatorContext* ctx) = 0;

    virtual void enterComparisonOperator(xCEPParser::ComparisonOperatorContext* ctx) = 0;
    virtual void exitComparisonOperator(xCEPParser::ComparisonOperatorContext* ctx) = 0;

    virtual void enterLogicalOperator(xCEPParser::LogicalOperatorContext* ctx) = 0;
    virtual void exitLogicalOperator(xCEPParser::LogicalOperatorContext* ctx) = 0;

    virtual void enterBitOperator(xCEPParser::BitOperatorContext* ctx) = 0;
    virtual void exitBitOperator(xCEPParser::BitOperatorContext* ctx) = 0;

    virtual void enterMathOperator(xCEPParser::MathOperatorContext* ctx) = 0;
    virtual void exitMathOperator(xCEPParser::MathOperatorContext* ctx) = 0;
};

}// namespace x::Parsers
#endif// x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPLISTENER_H_
