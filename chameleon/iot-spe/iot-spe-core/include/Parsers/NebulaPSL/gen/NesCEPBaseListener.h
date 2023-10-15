
// Generated from IoTDB/x-core/src/Parsers/IoTSPEPSL/gen/xCEP.g4 by ANTLR 4.9.2

#ifndef x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPBASELISTENER_H_
#define x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPBASELISTENER_H_

#include <Parsers/IoTSPEPSL/gen/xCEPListener.h>
#include <antlr4-runtime.h>

namespace x::Parsers {

/**
 * This class provides an empty implementation of xCEPListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class xCEPBaseListener : public xCEPListener {
  public:
    virtual void enterQuery(xCEPParser::QueryContext* /*ctx*/) override {}
    virtual void exitQuery(xCEPParser::QueryContext* /*ctx*/) override {}

    virtual void enterCepPattern(xCEPParser::CepPatternContext* /*ctx*/) override {}
    virtual void exitCepPattern(xCEPParser::CepPatternContext* /*ctx*/) override {}

    virtual void enterInputStreams(xCEPParser::InputStreamsContext* /*ctx*/) override {}
    virtual void exitInputStreams(xCEPParser::InputStreamsContext* /*ctx*/) override {}

    virtual void enterInputStream(xCEPParser::InputStreamContext* /*ctx*/) override {}
    virtual void exitInputStream(xCEPParser::InputStreamContext* /*ctx*/) override {}

    virtual void enterCompositeEventExpressions(xCEPParser::CompositeEventExpressionsContext* /*ctx*/) override {}
    virtual void exitCompositeEventExpressions(xCEPParser::CompositeEventExpressionsContext* /*ctx*/) override {}

    virtual void enterWhereExp(xCEPParser::WhereExpContext* /*ctx*/) override {}
    virtual void exitWhereExp(xCEPParser::WhereExpContext* /*ctx*/) override {}

    virtual void enterTimeConstraints(xCEPParser::TimeConstraintsContext* /*ctx*/) override {}
    virtual void exitTimeConstraints(xCEPParser::TimeConstraintsContext* /*ctx*/) override {}

    virtual void enterInterval(xCEPParser::IntervalContext* /*ctx*/) override {}
    virtual void exitInterval(xCEPParser::IntervalContext* /*ctx*/) override {}

    virtual void enterIntervalType(xCEPParser::IntervalTypeContext* /*ctx*/) override {}
    virtual void exitIntervalType(xCEPParser::IntervalTypeContext* /*ctx*/) override {}

    virtual void enterOption(xCEPParser::OptionContext* /*ctx*/) override {}
    virtual void exitOption(xCEPParser::OptionContext* /*ctx*/) override {}

    virtual void enterOutputExpression(xCEPParser::OutputExpressionContext* /*ctx*/) override {}
    virtual void exitOutputExpression(xCEPParser::OutputExpressionContext* /*ctx*/) override {}

    virtual void enterOutAttribute(xCEPParser::OutAttributeContext* /*ctx*/) override {}
    virtual void exitOutAttribute(xCEPParser::OutAttributeContext* /*ctx*/) override {}

    virtual void enterSinkList(xCEPParser::SinkListContext* /*ctx*/) override {}
    virtual void exitSinkList(xCEPParser::SinkListContext* /*ctx*/) override {}

    virtual void enterSink(xCEPParser::SinkContext* /*ctx*/) override {}
    virtual void exitSink(xCEPParser::SinkContext* /*ctx*/) override {}

    virtual void enterListEvents(xCEPParser::ListEventsContext* /*ctx*/) override {}
    virtual void exitListEvents(xCEPParser::ListEventsContext* /*ctx*/) override {}

    virtual void enterEventElem(xCEPParser::EventElemContext* /*ctx*/) override {}
    virtual void exitEventElem(xCEPParser::EventElemContext* /*ctx*/) override {}

    virtual void enterEvent(xCEPParser::EventContext* /*ctx*/) override {}
    virtual void exitEvent(xCEPParser::EventContext* /*ctx*/) override {}

    virtual void enterQuantifiers(xCEPParser::QuantifiersContext* /*ctx*/) override {}
    virtual void exitQuantifiers(xCEPParser::QuantifiersContext* /*ctx*/) override {}

    virtual void enterIterMax(xCEPParser::IterMaxContext* /*ctx*/) override {}
    virtual void exitIterMax(xCEPParser::IterMaxContext* /*ctx*/) override {}

    virtual void enterIterMin(xCEPParser::IterMinContext* /*ctx*/) override {}
    virtual void exitIterMin(xCEPParser::IterMinContext* /*ctx*/) override {}

    virtual void enterConsecutiveOption(xCEPParser::ConsecutiveOptionContext* /*ctx*/) override {}
    virtual void exitConsecutiveOption(xCEPParser::ConsecutiveOptionContext* /*ctx*/) override {}

    virtual void enterOperatorRule(xCEPParser::OperatorRuleContext* /*ctx*/) override {}
    virtual void exitOperatorRule(xCEPParser::OperatorRuleContext* /*ctx*/) override {}

    virtual void enterSequence(xCEPParser::SequenceContext* /*ctx*/) override {}
    virtual void exitSequence(xCEPParser::SequenceContext* /*ctx*/) override {}

    virtual void enterContiguity(xCEPParser::ContiguityContext* /*ctx*/) override {}
    virtual void exitContiguity(xCEPParser::ContiguityContext* /*ctx*/) override {}

    virtual void enterSinkType(xCEPParser::SinkTypeContext* /*ctx*/) override {}
    virtual void exitSinkType(xCEPParser::SinkTypeContext* /*ctx*/) override {}

    virtual void enterNullNotnull(xCEPParser::NullNotnullContext* /*ctx*/) override {}
    virtual void exitNullNotnull(xCEPParser::NullNotnullContext* /*ctx*/) override {}

    virtual void enterConstant(xCEPParser::ConstantContext* /*ctx*/) override {}
    virtual void exitConstant(xCEPParser::ConstantContext* /*ctx*/) override {}

    virtual void enterExpressions(xCEPParser::ExpressionsContext* /*ctx*/) override {}
    virtual void exitExpressions(xCEPParser::ExpressionsContext* /*ctx*/) override {}

    virtual void enterIsExpression(xCEPParser::IsExpressionContext* /*ctx*/) override {}
    virtual void exitIsExpression(xCEPParser::IsExpressionContext* /*ctx*/) override {}

    virtual void enterNotExpression(xCEPParser::NotExpressionContext* /*ctx*/) override {}
    virtual void exitNotExpression(xCEPParser::NotExpressionContext* /*ctx*/) override {}

    virtual void enterLogicalExpression(xCEPParser::LogicalExpressionContext* /*ctx*/) override {}
    virtual void exitLogicalExpression(xCEPParser::LogicalExpressionContext* /*ctx*/) override {}

    virtual void enterPredicateExpression(xCEPParser::PredicateExpressionContext* /*ctx*/) override {}
    virtual void exitPredicateExpression(xCEPParser::PredicateExpressionContext* /*ctx*/) override {}

    virtual void enterExpressionAtomPredicate(xCEPParser::ExpressionAtomPredicateContext* /*ctx*/) override {}
    virtual void exitExpressionAtomPredicate(xCEPParser::ExpressionAtomPredicateContext* /*ctx*/) override {}

    virtual void enterInPredicate(xCEPParser::InPredicateContext* /*ctx*/) override {}
    virtual void exitInPredicate(xCEPParser::InPredicateContext* /*ctx*/) override {}

    virtual void enterBinaryComparisonPredicate(xCEPParser::BinaryComparasionPredicateContext* /*ctx*/) override {}
    virtual void exitBinaryComparisonPredicate(xCEPParser::BinaryComparasionPredicateContext* /*ctx*/) override {}

    virtual void enterIsNullPredicate(xCEPParser::IsNullPredicateContext* /*ctx*/) override {}
    virtual void exitIsNullPredicate(xCEPParser::IsNullPredicateContext* /*ctx*/) override {}

    virtual void enterUnaryExpressionAtom(xCEPParser::UnaryExpressionAtomContext* /*ctx*/) override {}
    virtual void exitUnaryExpressionAtom(xCEPParser::UnaryExpressionAtomContext* /*ctx*/) override {}

    virtual void enterAttributeAtom(xCEPParser::AttributeAtomContext* /*ctx*/) override {}
    virtual void exitAttributeAtom(xCEPParser::AttributeAtomContext* /*ctx*/) override {}

    virtual void enterConstantExpressionAtom(xCEPParser::ConstantExpressionAtomContext* /*ctx*/) override {}
    virtual void exitConstantExpressionAtom(xCEPParser::ConstantExpressionAtomContext* /*ctx*/) override {}

    virtual void enterBinaryExpressionAtom(xCEPParser::BinaryExpressionAtomContext* /*ctx*/) override {}
    virtual void exitBinaryExpressionAtom(xCEPParser::BinaryExpressionAtomContext* /*ctx*/) override {}

    virtual void enterBitExpressionAtom(xCEPParser::BitExpressionAtomContext* /*ctx*/) override {}
    virtual void exitBitExpressionAtom(xCEPParser::BitExpressionAtomContext* /*ctx*/) override {}

    virtual void enterxtedExpressionAtom(xCEPParser::xtedExpressionAtomContext* /*ctx*/) override {}
    virtual void exitxtedExpressionAtom(xCEPParser::xtedExpressionAtomContext* /*ctx*/) override {}

    virtual void enterMathExpressionAtom(xCEPParser::MathExpressionAtomContext* /*ctx*/) override {}
    virtual void exitMathExpressionAtom(xCEPParser::MathExpressionAtomContext* /*ctx*/) override {}

    virtual void enterEventAttribute(xCEPParser::EventAttributeContext* /*ctx*/) override {}
    virtual void exitEventAttribute(xCEPParser::EventAttributeContext* /*ctx*/) override {}

    virtual void enterEventIteration(xCEPParser::EventIterationContext* /*ctx*/) override {}
    virtual void exitEventIteration(xCEPParser::EventIterationContext* /*ctx*/) override {}

    virtual void enterMathExpression(xCEPParser::MathExpressionContext* /*ctx*/) override {}
    virtual void exitMathExpression(xCEPParser::MathExpressionContext* /*ctx*/) override {}

    virtual void enterAggregation(xCEPParser::AggregationContext* /*ctx*/) override {}
    virtual void exitAggregation(xCEPParser::AggregationContext* /*ctx*/) override {}

    virtual void enterAttribute(xCEPParser::AttributeContext* /*ctx*/) override {}
    virtual void exitAttribute(xCEPParser::AttributeContext* /*ctx*/) override {}

    virtual void enterAttVal(xCEPParser::AttValContext* /*ctx*/) override {}
    virtual void exitAttVal(xCEPParser::AttValContext* /*ctx*/) override {}

    virtual void enterBoolRule(xCEPParser::BoolRuleContext* /*ctx*/) override {}
    virtual void exitBoolRule(xCEPParser::BoolRuleContext* /*ctx*/) override {}

    virtual void enterCondition(xCEPParser::ConditionContext* /*ctx*/) override {}
    virtual void exitCondition(xCEPParser::ConditionContext* /*ctx*/) override {}

    virtual void enterUnaryOperator(xCEPParser::UnaryOperatorContext* /*ctx*/) override {}
    virtual void exitUnaryOperator(xCEPParser::UnaryOperatorContext* /*ctx*/) override {}

    virtual void enterComparisonOperator(xCEPParser::ComparisonOperatorContext* /*ctx*/) override {}
    virtual void exitComparisonOperator(xCEPParser::ComparisonOperatorContext* /*ctx*/) override {}

    virtual void enterLogicalOperator(xCEPParser::LogicalOperatorContext* /*ctx*/) override {}
    virtual void exitLogicalOperator(xCEPParser::LogicalOperatorContext* /*ctx*/) override {}

    virtual void enterBitOperator(xCEPParser::BitOperatorContext* /*ctx*/) override {}
    virtual void exitBitOperator(xCEPParser::BitOperatorContext* /*ctx*/) override {}

    virtual void enterMathOperator(xCEPParser::MathOperatorContext* /*ctx*/) override {}
    virtual void exitMathOperator(xCEPParser::MathOperatorContext* /*ctx*/) override {}

    virtual void enterEveryRule(antlr4::ParserRuleContext* /*ctx*/) override {}
    virtual void exitEveryRule(antlr4::ParserRuleContext* /*ctx*/) override {}
    virtual void visitTerminal(antlr4::tree::TerminalNode* /*node*/) override {}
    virtual void visitErrorNode(antlr4::tree::ErrorNode* /*node*/) override {}
};

}// namespace x::Parsers
#endif// x_CORE_INCLUDE_PARSERS_IoTSPEPSL_GEN_xCEPBASELISTENER_H_
