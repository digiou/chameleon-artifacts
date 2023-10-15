
// Generated from IoTDB/x-core/src/Parsers/IoTSPEPSL/gen/xCEP.g4 by ANTLR 4.9.2

#include "Util/Logger/Logger.hpp"
#include <Parsers/IoTSPEPSL/gen/xCEPListener.h>
#include <Parsers/IoTSPEPSL/gen/xCEPParser.h>

using namespace antlrcpp;
using namespace x::Parsers;
using namespace antlr4;

xCEPParser::xCEPParser(TokenStream* input) : Parser(input) {
    _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

xCEPParser::~xCEPParser() { delete _interpreter; }

std::string xCEPParser::getGrammarFileName() const { return "xCEP.g4"; }

const std::vector<std::string>& xCEPParser::getRuleNames() const { return _ruleNames; }

dfa::Vocabulary& xCEPParser::getVocabulary() const { return _vocabulary; }

//----------------- QueryContext ------------------------------------------------------------------

xCEPParser::QueryContext::QueryContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::QueryContext::EOF() { return getToken(xCEPParser::EOF, 0); }

std::vector<xCEPParser::CepPatternContext*> xCEPParser::QueryContext::cepPattern() {
    return getRuleContexts<xCEPParser::CepPatternContext>();
}

xCEPParser::CepPatternContext* xCEPParser::QueryContext::cepPattern(size_t i) {
    return getRuleContext<xCEPParser::CepPatternContext>(i);
}

size_t xCEPParser::QueryContext::getRuleIndex() const { return xCEPParser::RuleQuery; }

void xCEPParser::QueryContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterQuery(this);
}

void xCEPParser::QueryContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitQuery(this);
}

xCEPParser::QueryContext* xCEPParser::query() {
    QueryContext* _localctx = _tracker.createInstance<QueryContext>(_ctx, getState());
    enterRule(_localctx, 0, xCEPParser::RuleQuery);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(89);
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
            setState(88);
            cepPattern();
            setState(91);
            _errHandler->sync(this);
            _la = _input->LA(1);
        } while (_la == xCEPParser::PATTERN);
        setState(93);
        match(xCEPParser::EOF);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- CepPatternContext ------------------------------------------------------------------

xCEPParser::CepPatternContext::CepPatternContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::CepPatternContext::PATTERN() { return getToken(xCEPParser::PATTERN, 0); }

tree::TerminalNode* xCEPParser::CepPatternContext::NAME() { return getToken(xCEPParser::NAME, 0); }

tree::TerminalNode* xCEPParser::CepPatternContext::SEP() { return getToken(xCEPParser::SEP, 0); }

xCEPParser::CompositeEventExpressionsContext* xCEPParser::CepPatternContext::compositeEventExpressions() {
    return getRuleContext<xCEPParser::CompositeEventExpressionsContext>(0);
}

tree::TerminalNode* xCEPParser::CepPatternContext::FROM() { return getToken(xCEPParser::FROM, 0); }

xCEPParser::InputStreamsContext* xCEPParser::CepPatternContext::inputStreams() {
    return getRuleContext<xCEPParser::InputStreamsContext>(0);
}

tree::TerminalNode* xCEPParser::CepPatternContext::INTO() { return getToken(xCEPParser::INTO, 0); }

xCEPParser::SinkListContext* xCEPParser::CepPatternContext::sinkList() {
    return getRuleContext<xCEPParser::SinkListContext>(0);
}

tree::TerminalNode* xCEPParser::CepPatternContext::WHERE() { return getToken(xCEPParser::WHERE, 0); }

xCEPParser::WhereExpContext* xCEPParser::CepPatternContext::whereExp() {
    return getRuleContext<xCEPParser::WhereExpContext>(0);
}

tree::TerminalNode* xCEPParser::CepPatternContext::WITHIN() { return getToken(xCEPParser::WITHIN, 0); }

xCEPParser::TimeConstraintsContext* xCEPParser::CepPatternContext::timeConstraints() {
    return getRuleContext<xCEPParser::TimeConstraintsContext>(0);
}

tree::TerminalNode* xCEPParser::CepPatternContext::CONSUMING() { return getToken(xCEPParser::CONSUMING, 0); }

xCEPParser::OptionContext* xCEPParser::CepPatternContext::option() { return getRuleContext<xCEPParser::OptionContext>(0); }

tree::TerminalNode* xCEPParser::CepPatternContext::RETURN() { return getToken(xCEPParser::RETURN, 0); }

xCEPParser::OutputExpressionContext* xCEPParser::CepPatternContext::outputExpression() {
    return getRuleContext<xCEPParser::OutputExpressionContext>(0);
}

size_t xCEPParser::CepPatternContext::getRuleIndex() const { return xCEPParser::RuleCepPattern; }

void xCEPParser::CepPatternContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterCepPattern(this);
}

void xCEPParser::CepPatternContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitCepPattern(this);
}

xCEPParser::CepPatternContext* xCEPParser::cepPattern() {
    CepPatternContext* _localctx = _tracker.createInstance<CepPatternContext>(_ctx, getState());
    enterRule(_localctx, 2, xCEPParser::RuleCepPattern);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(95);
        match(xCEPParser::PATTERN);
        setState(96);
        match(xCEPParser::NAME);
        setState(97);
        match(xCEPParser::SEP);
        setState(98);
        compositeEventExpressions();
        setState(99);
        match(xCEPParser::FROM);
        setState(100);
        inputStreams();
        setState(103);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::WHERE) {
            setState(101);
            match(xCEPParser::WHERE);
            setState(102);
            whereExp();
        }
        setState(107);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::WITHIN) {
            setState(105);
            match(xCEPParser::WITHIN);
            setState(106);
            timeConstraints();
        }
        setState(111);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::CONSUMING) {
            setState(109);
            match(xCEPParser::CONSUMING);
            setState(110);
            option();
        }
        setState(115);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::RETURN) {
            setState(113);
            match(xCEPParser::RETURN);
            setState(114);
            outputExpression();
        }
        setState(117);
        match(xCEPParser::INTO);
        setState(118);
        sinkList();

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- InputStreamsContext ------------------------------------------------------------------

xCEPParser::InputStreamsContext::InputStreamsContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<xCEPParser::InputStreamContext*> xCEPParser::InputStreamsContext::inputStream() {
    return getRuleContexts<xCEPParser::InputStreamContext>();
}

xCEPParser::InputStreamContext* xCEPParser::InputStreamsContext::inputStream(size_t i) {
    return getRuleContext<xCEPParser::InputStreamContext>(i);
}

std::vector<tree::TerminalNode*> xCEPParser::InputStreamsContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::InputStreamsContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

size_t xCEPParser::InputStreamsContext::getRuleIndex() const { return xCEPParser::RuleInputStreams; }

void xCEPParser::InputStreamsContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterInputStreams(this);
}

void xCEPParser::InputStreamsContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitInputStreams(this);
}

xCEPParser::InputStreamsContext* xCEPParser::inputStreams() {
    InputStreamsContext* _localctx = _tracker.createInstance<InputStreamsContext>(_ctx, getState());
    enterRule(_localctx, 4, xCEPParser::RuleInputStreams);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(120);
        inputStream();
        setState(125);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == xCEPParser::COMMA) {
            setState(121);
            match(xCEPParser::COMMA);
            setState(122);
            inputStream();
            setState(127);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- InputStreamContext ------------------------------------------------------------------

xCEPParser::InputStreamContext::InputStreamContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<tree::TerminalNode*> xCEPParser::InputStreamContext::NAME() { return getTokens(xCEPParser::NAME); }

tree::TerminalNode* xCEPParser::InputStreamContext::NAME(size_t i) { return getToken(xCEPParser::NAME, i); }

tree::TerminalNode* xCEPParser::InputStreamContext::AS() { return getToken(xCEPParser::AS, 0); }

size_t xCEPParser::InputStreamContext::getRuleIndex() const { return xCEPParser::RuleInputStream; }

void xCEPParser::InputStreamContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterInputStream(this);
}

void xCEPParser::InputStreamContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitInputStream(this);
}

xCEPParser::InputStreamContext* xCEPParser::inputStream() {
    InputStreamContext* _localctx = _tracker.createInstance<InputStreamContext>(_ctx, getState());
    enterRule(_localctx, 6, xCEPParser::RuleInputStream);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(128);
        match(xCEPParser::NAME);
        setState(131);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::AS) {
            setState(129);
            match(xCEPParser::AS);
            setState(130);
            match(xCEPParser::NAME);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- CompositeEventExpressionsContext ------------------------------------------------------------------

xCEPParser::CompositeEventExpressionsContext::CompositeEventExpressionsContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::CompositeEventExpressionsContext::LPARENTHESIS() {
    return getToken(xCEPParser::LPARENTHESIS, 0);
}

xCEPParser::ListEventsContext* xCEPParser::CompositeEventExpressionsContext::listEvents() {
    return getRuleContext<xCEPParser::ListEventsContext>(0);
}

tree::TerminalNode* xCEPParser::CompositeEventExpressionsContext::RPARENTHESIS() {
    return getToken(xCEPParser::RPARENTHESIS, 0);
}

size_t xCEPParser::CompositeEventExpressionsContext::getRuleIndex() const {
    return xCEPParser::RuleCompositeEventExpressions;
}

void xCEPParser::CompositeEventExpressionsContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterCompositeEventExpressions(this);
}

void xCEPParser::CompositeEventExpressionsContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitCompositeEventExpressions(this);
}

xCEPParser::CompositeEventExpressionsContext* xCEPParser::compositeEventExpressions() {
    CompositeEventExpressionsContext* _localctx = _tracker.createInstance<CompositeEventExpressionsContext>(_ctx, getState());
    enterRule(_localctx, 8, xCEPParser::RuleCompositeEventExpressions);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(133);
        match(xCEPParser::LPARENTHESIS);
        setState(134);
        listEvents();
        setState(135);
        match(xCEPParser::RPARENTHESIS);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- WhereExpContext ------------------------------------------------------------------

xCEPParser::WhereExpContext::WhereExpContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

xCEPParser::ExpressionContext* xCEPParser::WhereExpContext::expression() {
    return getRuleContext<xCEPParser::ExpressionContext>(0);
}

size_t xCEPParser::WhereExpContext::getRuleIndex() const { return xCEPParser::RuleWhereExp; }

void xCEPParser::WhereExpContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterWhereExp(this);
}

void xCEPParser::WhereExpContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitWhereExp(this);
}

xCEPParser::WhereExpContext* xCEPParser::whereExp() {
    WhereExpContext* _localctx = _tracker.createInstance<WhereExpContext>(_ctx, getState());
    enterRule(_localctx, 10, xCEPParser::RuleWhereExp);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(137);
        expression(0);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- TimeConstraintsContext ------------------------------------------------------------------

xCEPParser::TimeConstraintsContext::TimeConstraintsContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::TimeConstraintsContext::LBRACKET() { return getToken(xCEPParser::LBRACKET, 0); }

xCEPParser::IntervalContext* xCEPParser::TimeConstraintsContext::interval() {
    return getRuleContext<xCEPParser::IntervalContext>(0);
}

tree::TerminalNode* xCEPParser::TimeConstraintsContext::RBRACKET() { return getToken(xCEPParser::RBRACKET, 0); }

size_t xCEPParser::TimeConstraintsContext::getRuleIndex() const { return xCEPParser::RuleTimeConstraints; }

void xCEPParser::TimeConstraintsContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterTimeConstraints(this);
}

void xCEPParser::TimeConstraintsContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitTimeConstraints(this);
}

xCEPParser::TimeConstraintsContext* xCEPParser::timeConstraints() {
    TimeConstraintsContext* _localctx = _tracker.createInstance<TimeConstraintsContext>(_ctx, getState());
    enterRule(_localctx, 12, xCEPParser::RuleTimeConstraints);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(139);
        match(xCEPParser::LBRACKET);
        setState(140);
        interval();
        setState(141);
        match(xCEPParser::RBRACKET);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- IntervalContext ------------------------------------------------------------------

xCEPParser::IntervalContext::IntervalContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::IntervalContext::INT() { return getToken(xCEPParser::INT, 0); }

xCEPParser::IntervalTypeContext* xCEPParser::IntervalContext::intervalType() {
    return getRuleContext<xCEPParser::IntervalTypeContext>(0);
}

size_t xCEPParser::IntervalContext::getRuleIndex() const { return xCEPParser::RuleInterval; }

void xCEPParser::IntervalContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterInterval(this);
}

void xCEPParser::IntervalContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitInterval(this);
}

xCEPParser::IntervalContext* xCEPParser::interval() {
    IntervalContext* _localctx = _tracker.createInstance<IntervalContext>(_ctx, getState());
    enterRule(_localctx, 14, xCEPParser::RuleInterval);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(143);
        match(xCEPParser::INT);
        setState(144);
        intervalType();

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- IntervalTypeContext ------------------------------------------------------------------

xCEPParser::IntervalTypeContext::IntervalTypeContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::IntervalTypeContext::QUARTER() { return getToken(xCEPParser::QUARTER, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::MONTH() { return getToken(xCEPParser::MONTH, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::DAY() { return getToken(xCEPParser::DAY, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::HOUR() { return getToken(xCEPParser::HOUR, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::MINUTE() { return getToken(xCEPParser::MINUTE, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::WEEK() { return getToken(xCEPParser::WEEK, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::SECOND() { return getToken(xCEPParser::SECOND, 0); }

tree::TerminalNode* xCEPParser::IntervalTypeContext::MICROSECOND() { return getToken(xCEPParser::MICROSECOND, 0); }

size_t xCEPParser::IntervalTypeContext::getRuleIndex() const { return xCEPParser::RuleIntervalType; }

void xCEPParser::IntervalTypeContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterIntervalType(this);
}

void xCEPParser::IntervalTypeContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitIntervalType(this);
}

xCEPParser::IntervalTypeContext* xCEPParser::intervalType() {
    IntervalTypeContext* _localctx = _tracker.createInstance<IntervalTypeContext>(_ctx, getState());
    enterRule(_localctx, 16, xCEPParser::RuleIntervalType);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(146);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0)
               && ((1ULL << _la)
                   & ((1ULL << xCEPParser::QUARTER) | (1ULL << xCEPParser::MONTH) | (1ULL << xCEPParser::DAY)
                      | (1ULL << xCEPParser::HOUR) | (1ULL << xCEPParser::MINUTE) | (1ULL << xCEPParser::WEEK)
                      | (1ULL << xCEPParser::SECOND) | (1ULL << xCEPParser::MICROSECOND)))
                   != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- OptionContext ------------------------------------------------------------------

xCEPParser::OptionContext::OptionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::OptionContext::ALL() { return getToken(xCEPParser::ALL, 0); }

tree::TerminalNode* xCEPParser::OptionContext::NONE() { return getToken(xCEPParser::NONE, 0); }

size_t xCEPParser::OptionContext::getRuleIndex() const { return xCEPParser::RuleOption; }

void xCEPParser::OptionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterOption(this);
}

void xCEPParser::OptionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitOption(this);
}

xCEPParser::OptionContext* xCEPParser::option() {
    OptionContext* _localctx = _tracker.createInstance<OptionContext>(_ctx, getState());
    enterRule(_localctx, 18, xCEPParser::RuleOption);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(148);
        _la = _input->LA(1);
        if (!(_la == xCEPParser::ALL

              || _la == xCEPParser::NONE)) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- OutputExpressionContext ------------------------------------------------------------------

xCEPParser::OutputExpressionContext::OutputExpressionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::OutputExpressionContext::NAME() { return getToken(xCEPParser::NAME, 0); }

tree::TerminalNode* xCEPParser::OutputExpressionContext::SEP() { return getToken(xCEPParser::SEP, 0); }

tree::TerminalNode* xCEPParser::OutputExpressionContext::LBRACKET() { return getToken(xCEPParser::LBRACKET, 0); }

std::vector<xCEPParser::OutAttributeContext*> xCEPParser::OutputExpressionContext::outAttribute() {
    return getRuleContexts<xCEPParser::OutAttributeContext>();
}

xCEPParser::OutAttributeContext* xCEPParser::OutputExpressionContext::outAttribute(size_t i) {
    return getRuleContext<xCEPParser::OutAttributeContext>(i);
}

tree::TerminalNode* xCEPParser::OutputExpressionContext::RBRACKET() { return getToken(xCEPParser::RBRACKET, 0); }

std::vector<tree::TerminalNode*> xCEPParser::OutputExpressionContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::OutputExpressionContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

size_t xCEPParser::OutputExpressionContext::getRuleIndex() const { return xCEPParser::RuleOutputExpression; }

void xCEPParser::OutputExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterOutputExpression(this);
}

void xCEPParser::OutputExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitOutputExpression(this);
}

xCEPParser::OutputExpressionContext* xCEPParser::outputExpression() {
    OutputExpressionContext* _localctx = _tracker.createInstance<OutputExpressionContext>(_ctx, getState());
    enterRule(_localctx, 20, xCEPParser::RuleOutputExpression);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(150);
        match(xCEPParser::NAME);
        setState(151);
        match(xCEPParser::SEP);
        setState(152);
        match(xCEPParser::LBRACKET);
        setState(153);
        outAttribute();
        setState(158);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == xCEPParser::COMMA) {
            setState(154);
            match(xCEPParser::COMMA);
            setState(155);
            outAttribute();
            setState(160);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(161);
        match(xCEPParser::RBRACKET);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- OutAttributeContext ------------------------------------------------------------------

xCEPParser::OutAttributeContext::OutAttributeContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::OutAttributeContext::NAME() { return getToken(xCEPParser::NAME, 0); }

tree::TerminalNode* xCEPParser::OutAttributeContext::EQUAL() { return getToken(xCEPParser::EQUAL, 0); }

xCEPParser::AttValContext* xCEPParser::OutAttributeContext::attVal() {
    return getRuleContext<xCEPParser::AttValContext>(0);
}

size_t xCEPParser::OutAttributeContext::getRuleIndex() const { return xCEPParser::RuleOutAttribute; }

void xCEPParser::OutAttributeContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterOutAttribute(this);
}

void xCEPParser::OutAttributeContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitOutAttribute(this);
}

xCEPParser::OutAttributeContext* xCEPParser::outAttribute() {
    OutAttributeContext* _localctx = _tracker.createInstance<OutAttributeContext>(_ctx, getState());
    enterRule(_localctx, 22, xCEPParser::RuleOutAttribute);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(163);
        match(xCEPParser::NAME);
        setState(164);
        match(xCEPParser::EQUAL);
        setState(165);
        attVal();

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- SinkListContext ------------------------------------------------------------------

xCEPParser::SinkListContext::SinkListContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<xCEPParser::SinkContext*> xCEPParser::SinkListContext::sink() {
    return getRuleContexts<xCEPParser::SinkContext>();
}

xCEPParser::SinkContext* xCEPParser::SinkListContext::sink(size_t i) { return getRuleContext<xCEPParser::SinkContext>(i); }

std::vector<tree::TerminalNode*> xCEPParser::SinkListContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::SinkListContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

size_t xCEPParser::SinkListContext::getRuleIndex() const { return xCEPParser::RuleSinkList; }

void xCEPParser::SinkListContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterSinkList(this);
}

void xCEPParser::SinkListContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitSinkList(this);
}

xCEPParser::SinkListContext* xCEPParser::sinkList() {
    SinkListContext* _localctx = _tracker.createInstance<SinkListContext>(_ctx, getState());
    enterRule(_localctx, 24, xCEPParser::RuleSinkList);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(167);
        sink();
        setState(172);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == xCEPParser::COMMA) {
            setState(168);
            match(xCEPParser::COMMA);
            setState(169);
            sink();
            setState(174);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- SinkContext ------------------------------------------------------------------

xCEPParser::SinkContext::SinkContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

xCEPParser::SinkTypeContext* xCEPParser::SinkContext::sinkType() { return getRuleContext<xCEPParser::SinkTypeContext>(0); }

tree::TerminalNode* xCEPParser::SinkContext::SINKSEP() { return getToken(xCEPParser::SINKSEP, 0); }

tree::TerminalNode* xCEPParser::SinkContext::NAME() { return getToken(xCEPParser::NAME, 0); }

size_t xCEPParser::SinkContext::getRuleIndex() const { return xCEPParser::RuleSink; }

void xCEPParser::SinkContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterSink(this);
}

void xCEPParser::SinkContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitSink(this);
}

xCEPParser::SinkContext* xCEPParser::sink() {
    SinkContext* _localctx = _tracker.createInstance<SinkContext>(_ctx, getState());
    enterRule(_localctx, 26, xCEPParser::RuleSink);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(175);
        sinkType();
        setState(176);
        match(xCEPParser::SINKSEP);
        setState(177);
        match(xCEPParser::NAME);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ListEventsContext ------------------------------------------------------------------

xCEPParser::ListEventsContext::ListEventsContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<xCEPParser::EventElemContext*> xCEPParser::ListEventsContext::eventElem() {
    return getRuleContexts<xCEPParser::EventElemContext>();
}

xCEPParser::EventElemContext* xCEPParser::ListEventsContext::eventElem(size_t i) {
    return getRuleContext<xCEPParser::EventElemContext>(i);
}

std::vector<xCEPParser::OperatorRuleContext*> xCEPParser::ListEventsContext::operatorRule() {
    return getRuleContexts<xCEPParser::OperatorRuleContext>();
}

xCEPParser::OperatorRuleContext* xCEPParser::ListEventsContext::operatorRule(size_t i) {
    return getRuleContext<xCEPParser::OperatorRuleContext>(i);
}

size_t xCEPParser::ListEventsContext::getRuleIndex() const { return xCEPParser::RuleListEvents; }

void xCEPParser::ListEventsContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterListEvents(this);
}

void xCEPParser::ListEventsContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitListEvents(this);
}

xCEPParser::ListEventsContext* xCEPParser::listEvents() {
    ListEventsContext* _localctx = _tracker.createInstance<ListEventsContext>(_ctx, getState());
    enterRule(_localctx, 28, xCEPParser::RuleListEvents);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(179);
        eventElem();
        setState(185);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~0x3fULL) == 0)
                && ((1ULL << _la)
                    & ((1ULL << xCEPParser::ANY) | (1ULL << xCEPParser::SEQ) | (1ULL << xCEPParser::NEXT)
                       | (1ULL << xCEPParser::AND) | (1ULL << xCEPParser::OR)))
                    != 0)) {
            setState(180);
            operatorRule();
            setState(181);
            eventElem();
            setState(187);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- EventElemContext ------------------------------------------------------------------

xCEPParser::EventElemContext::EventElemContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

xCEPParser::EventContext* xCEPParser::EventElemContext::event() { return getRuleContext<xCEPParser::EventContext>(0); }

tree::TerminalNode* xCEPParser::EventElemContext::NOT() { return getToken(xCEPParser::NOT, 0); }

tree::TerminalNode* xCEPParser::EventElemContext::LPARENTHESIS() { return getToken(xCEPParser::LPARENTHESIS, 0); }

xCEPParser::ListEventsContext* xCEPParser::EventElemContext::listEvents() {
    return getRuleContext<xCEPParser::ListEventsContext>(0);
}

tree::TerminalNode* xCEPParser::EventElemContext::RPARENTHESIS() { return getToken(xCEPParser::RPARENTHESIS, 0); }

size_t xCEPParser::EventElemContext::getRuleIndex() const { return xCEPParser::RuleEventElem; }

void xCEPParser::EventElemContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterEventElem(this);
}

void xCEPParser::EventElemContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitEventElem(this);
}

xCEPParser::EventElemContext* xCEPParser::eventElem() {
    EventElemContext* _localctx = _tracker.createInstance<EventElemContext>(_ctx, getState());
    enterRule(_localctx, 30, xCEPParser::RuleEventElem);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(199);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(189);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::NOT) {
                    setState(188);
                    match(xCEPParser::NOT);
                }
                setState(191);
                event();
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(193);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::NOT) {
                    setState(192);
                    match(xCEPParser::NOT);
                }
                setState(195);
                match(xCEPParser::LPARENTHESIS);
                setState(196);
                listEvents();
                setState(197);
                match(xCEPParser::RPARENTHESIS);
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- EventContext ------------------------------------------------------------------

xCEPParser::EventContext::EventContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::EventContext::NAME() { return getToken(xCEPParser::NAME, 0); }

xCEPParser::QuantifiersContext* xCEPParser::EventContext::quantifiers() {
    return getRuleContext<xCEPParser::QuantifiersContext>(0);
}

size_t xCEPParser::EventContext::getRuleIndex() const { return xCEPParser::RuleEvent; }

void xCEPParser::EventContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterEvent(this);
}

void xCEPParser::EventContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitEvent(this);
}

xCEPParser::EventContext* xCEPParser::event() {
    EventContext* _localctx = _tracker.createInstance<EventContext>(_ctx, getState());
    enterRule(_localctx, 32, xCEPParser::RuleEvent);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(201);
        match(xCEPParser::NAME);
        setState(203);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~0x3fULL) == 0)
             && ((1ULL << _la) & ((1ULL << xCEPParser::STAR) | (1ULL << xCEPParser::PLUS) | (1ULL << xCEPParser::LBRACKET)))
                 != 0)) {
            setState(202);
            quantifiers();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- QuantifiersContext ------------------------------------------------------------------

xCEPParser::QuantifiersContext::QuantifiersContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::QuantifiersContext::STAR() { return getToken(xCEPParser::STAR, 0); }

tree::TerminalNode* xCEPParser::QuantifiersContext::PLUS() { return getToken(xCEPParser::PLUS, 0); }

tree::TerminalNode* xCEPParser::QuantifiersContext::LBRACKET() { return getToken(xCEPParser::LBRACKET, 0); }

tree::TerminalNode* xCEPParser::QuantifiersContext::INT() { return getToken(xCEPParser::INT, 0); }

tree::TerminalNode* xCEPParser::QuantifiersContext::RBRACKET() { return getToken(xCEPParser::RBRACKET, 0); }

xCEPParser::ConsecutiveOptionContext* xCEPParser::QuantifiersContext::consecutiveOption() {
    return getRuleContext<xCEPParser::ConsecutiveOptionContext>(0);
}

xCEPParser::IterMinContext* xCEPParser::QuantifiersContext::iterMin() {
    return getRuleContext<xCEPParser::IterMinContext>(0);
}

tree::TerminalNode* xCEPParser::QuantifiersContext::D_POINTS() { return getToken(xCEPParser::D_POINTS, 0); }

xCEPParser::IterMaxContext* xCEPParser::QuantifiersContext::iterMax() {
    return getRuleContext<xCEPParser::IterMaxContext>(0);
}

size_t xCEPParser::QuantifiersContext::getRuleIndex() const { return xCEPParser::RuleQuantifiers; }

void xCEPParser::QuantifiersContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterQuantifiers(this);
}

void xCEPParser::QuantifiersContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitQuantifiers(this);
}

xCEPParser::QuantifiersContext* xCEPParser::quantifiers() {
    QuantifiersContext* _localctx = _tracker.createInstance<QuantifiersContext>(_ctx, getState());
    enterRule(_localctx, 34, xCEPParser::RuleQuantifiers);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(225);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(205);
                match(xCEPParser::STAR);
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(206);
                match(xCEPParser::PLUS);
                break;
            }

            case 3: {
                enterOuterAlt(_localctx, 3);
                setState(207);
                match(xCEPParser::LBRACKET);
                setState(209);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::ANY

                    || _la == xCEPParser::NEXT) {
                    setState(208);
                    consecutiveOption();
                }
                setState(211);
                match(xCEPParser::INT);
                setState(213);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::PLUS) {
                    setState(212);
                    match(xCEPParser::PLUS);
                }
                setState(215);
                match(xCEPParser::RBRACKET);
                break;
            }

            case 4: {
                enterOuterAlt(_localctx, 4);
                setState(216);
                match(xCEPParser::LBRACKET);
                setState(218);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::ANY

                    || _la == xCEPParser::NEXT) {
                    setState(217);
                    consecutiveOption();
                }
                setState(220);
                iterMin();
                setState(221);
                match(xCEPParser::D_POINTS);
                setState(222);
                iterMax();
                setState(223);
                match(xCEPParser::RBRACKET);
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- IterMaxContext ------------------------------------------------------------------

xCEPParser::IterMaxContext::IterMaxContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::IterMaxContext::INT() { return getToken(xCEPParser::INT, 0); }

size_t xCEPParser::IterMaxContext::getRuleIndex() const { return xCEPParser::RuleIterMax; }

void xCEPParser::IterMaxContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterIterMax(this);
}

void xCEPParser::IterMaxContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitIterMax(this);
}

xCEPParser::IterMaxContext* xCEPParser::iterMax() {
    IterMaxContext* _localctx = _tracker.createInstance<IterMaxContext>(_ctx, getState());
    enterRule(_localctx, 36, xCEPParser::RuleIterMax);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(227);
        match(xCEPParser::INT);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- IterMinContext ------------------------------------------------------------------

xCEPParser::IterMinContext::IterMinContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::IterMinContext::INT() { return getToken(xCEPParser::INT, 0); }

size_t xCEPParser::IterMinContext::getRuleIndex() const { return xCEPParser::RuleIterMin; }

void xCEPParser::IterMinContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterIterMin(this);
}

void xCEPParser::IterMinContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitIterMin(this);
}

xCEPParser::IterMinContext* xCEPParser::iterMin() {
    IterMinContext* _localctx = _tracker.createInstance<IterMinContext>(_ctx, getState());
    enterRule(_localctx, 38, xCEPParser::RuleIterMin);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(229);
        match(xCEPParser::INT);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConsecutiveOptionContext ------------------------------------------------------------------

xCEPParser::ConsecutiveOptionContext::ConsecutiveOptionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::ConsecutiveOptionContext::NEXT() { return getToken(xCEPParser::NEXT, 0); }

tree::TerminalNode* xCEPParser::ConsecutiveOptionContext::ANY() { return getToken(xCEPParser::ANY, 0); }

size_t xCEPParser::ConsecutiveOptionContext::getRuleIndex() const { return xCEPParser::RuleConsecutiveOption; }

void xCEPParser::ConsecutiveOptionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterConsecutiveOption(this);
}

void xCEPParser::ConsecutiveOptionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitConsecutiveOption(this);
}

xCEPParser::ConsecutiveOptionContext* xCEPParser::consecutiveOption() {
    ConsecutiveOptionContext* _localctx = _tracker.createInstance<ConsecutiveOptionContext>(_ctx, getState());
    enterRule(_localctx, 40, xCEPParser::RuleConsecutiveOption);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(232);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::ANY) {
            setState(231);
            match(xCEPParser::ANY);
        }
        setState(234);
        match(xCEPParser::NEXT);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- OperatorRuleContext ------------------------------------------------------------------

xCEPParser::OperatorRuleContext::OperatorRuleContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::OperatorRuleContext::AND() { return getToken(xCEPParser::AND, 0); }

tree::TerminalNode* xCEPParser::OperatorRuleContext::OR() { return getToken(xCEPParser::OR, 0); }

xCEPParser::SequenceContext* xCEPParser::OperatorRuleContext::sequence() {
    return getRuleContext<xCEPParser::SequenceContext>(0);
}

size_t xCEPParser::OperatorRuleContext::getRuleIndex() const { return xCEPParser::RuleOperatorRule; }

void xCEPParser::OperatorRuleContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterOperatorRule(this);
}

void xCEPParser::OperatorRuleContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitOperatorRule(this);
}

xCEPParser::OperatorRuleContext* xCEPParser::operatorRule() {
    OperatorRuleContext* _localctx = _tracker.createInstance<OperatorRuleContext>(_ctx, getState());
    enterRule(_localctx, 42, xCEPParser::RuleOperatorRule);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(239);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::AND: {
                enterOuterAlt(_localctx, 1);
                setState(236);
                match(xCEPParser::AND);
                break;
            }

            case xCEPParser::OR: {
                enterOuterAlt(_localctx, 2);
                setState(237);
                match(xCEPParser::OR);
                break;
            }

            case xCEPParser::ANY:
            case xCEPParser::SEQ:
            case xCEPParser::NEXT: {
                enterOuterAlt(_localctx, 3);
                setState(238);
                sequence();
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- SequenceContext ------------------------------------------------------------------

xCEPParser::SequenceContext::SequenceContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::SequenceContext::SEQ() { return getToken(xCEPParser::SEQ, 0); }

xCEPParser::ContiguityContext* xCEPParser::SequenceContext::contiguity() {
    return getRuleContext<xCEPParser::ContiguityContext>(0);
}

size_t xCEPParser::SequenceContext::getRuleIndex() const { return xCEPParser::RuleSequence; }

void xCEPParser::SequenceContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterSequence(this);
}

void xCEPParser::SequenceContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitSequence(this);
}

xCEPParser::SequenceContext* xCEPParser::sequence() {
    SequenceContext* _localctx = _tracker.createInstance<SequenceContext>(_ctx, getState());
    enterRule(_localctx, 44, xCEPParser::RuleSequence);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(243);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::SEQ: {
                enterOuterAlt(_localctx, 1);
                setState(241);
                match(xCEPParser::SEQ);
                break;
            }

            case xCEPParser::ANY:
            case xCEPParser::NEXT: {
                enterOuterAlt(_localctx, 2);
                setState(242);
                contiguity();
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ContiguityContext ------------------------------------------------------------------

xCEPParser::ContiguityContext::ContiguityContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::ContiguityContext::NEXT() { return getToken(xCEPParser::NEXT, 0); }

tree::TerminalNode* xCEPParser::ContiguityContext::ANY() { return getToken(xCEPParser::ANY, 0); }

size_t xCEPParser::ContiguityContext::getRuleIndex() const { return xCEPParser::RuleContiguity; }

void xCEPParser::ContiguityContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterContiguity(this);
}

void xCEPParser::ContiguityContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitContiguity(this);
}

xCEPParser::ContiguityContext* xCEPParser::contiguity() {
    ContiguityContext* _localctx = _tracker.createInstance<ContiguityContext>(_ctx, getState());
    enterRule(_localctx, 46, xCEPParser::RuleContiguity);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(248);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::NEXT: {
                enterOuterAlt(_localctx, 1);
                setState(245);
                match(xCEPParser::NEXT);
                break;
            }

            case xCEPParser::ANY: {
                enterOuterAlt(_localctx, 2);
                setState(246);
                match(xCEPParser::ANY);
                setState(247);
                match(xCEPParser::NEXT);
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- SinkTypeContext ------------------------------------------------------------------

xCEPParser::SinkTypeContext::SinkTypeContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::SinkTypeContext::KAFKA() { return getToken(xCEPParser::KAFKA, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::FILE() { return getToken(xCEPParser::FILE, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::MQTT() { return getToken(xCEPParser::MQTT, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::NETWORK() { return getToken(xCEPParser::NETWORK, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::NULLOUTPUT() { return getToken(xCEPParser::NULLOUTPUT, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::OPC() { return getToken(xCEPParser::OPC, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::PRINT() { return getToken(xCEPParser::PRINT, 0); }

tree::TerminalNode* xCEPParser::SinkTypeContext::ZMQ() { return getToken(xCEPParser::ZMQ, 0); }

size_t xCEPParser::SinkTypeContext::getRuleIndex() const { return xCEPParser::RuleSinkType; }

void xCEPParser::SinkTypeContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterSinkType(this);
}

void xCEPParser::SinkTypeContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitSinkType(this);
}

xCEPParser::SinkTypeContext* xCEPParser::sinkType() {
    SinkTypeContext* _localctx = _tracker.createInstance<SinkTypeContext>(_ctx, getState());
    enterRule(_localctx, 48, xCEPParser::RuleSinkType);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(250);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0)
               && ((1ULL << _la)
                   & ((1ULL << xCEPParser::KAFKA) | (1ULL << xCEPParser::FILE) | (1ULL << xCEPParser::MQTT)
                      | (1ULL << xCEPParser::NETWORK) | (1ULL << xCEPParser::NULLOUTPUT) | (1ULL << xCEPParser::OPC)
                      | (1ULL << xCEPParser::PRINT) | (1ULL << xCEPParser::ZMQ)))
                   != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- NullNotnullContext ------------------------------------------------------------------

xCEPParser::NullNotnullContext::NullNotnullContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::NullNotnullContext::NULLTOKEN() { return getToken(xCEPParser::NULLTOKEN, 0); }

tree::TerminalNode* xCEPParser::NullNotnullContext::NOT() { return getToken(xCEPParser::NOT, 0); }

size_t xCEPParser::NullNotnullContext::getRuleIndex() const { return xCEPParser::RuleNullNotnull; }

void xCEPParser::NullNotnullContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterNullNotnull(this);
}

void xCEPParser::NullNotnullContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitNullNotnull(this);
}

xCEPParser::NullNotnullContext* xCEPParser::nullNotnull() {
    NullNotnullContext* _localctx = _tracker.createInstance<NullNotnullContext>(_ctx, getState());
    enterRule(_localctx, 50, xCEPParser::RuleNullNotnull);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(253);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == xCEPParser::NOT) {
            setState(252);
            match(xCEPParser::NOT);
        }
        setState(255);
        match(xCEPParser::NULLTOKEN);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConstantContext ------------------------------------------------------------------

xCEPParser::ConstantContext::ConstantContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<tree::TerminalNode*> xCEPParser::ConstantContext::QUOTE() { return getTokens(xCEPParser::QUOTE); }

tree::TerminalNode* xCEPParser::ConstantContext::QUOTE(size_t i) { return getToken(xCEPParser::QUOTE, i); }

tree::TerminalNode* xCEPParser::ConstantContext::NAME() { return getToken(xCEPParser::NAME, 0); }

tree::TerminalNode* xCEPParser::ConstantContext::INT() { return getToken(xCEPParser::INT, 0); }

size_t xCEPParser::ConstantContext::getRuleIndex() const { return xCEPParser::RuleConstant; }

void xCEPParser::ConstantContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterConstant(this);
}

void xCEPParser::ConstantContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitConstant(this);
}

xCEPParser::ConstantContext* xCEPParser::constant() {
    ConstantContext* _localctx = _tracker.createInstance<ConstantContext>(_ctx, getState());
    enterRule(_localctx, 52, xCEPParser::RuleConstant);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(262);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::QUOTE: {
                enterOuterAlt(_localctx, 1);
                setState(257);
                match(xCEPParser::QUOTE);
                setState(258);
                match(xCEPParser::NAME);
                setState(259);
                match(xCEPParser::QUOTE);
                break;
            }

            case xCEPParser::INT: {
                enterOuterAlt(_localctx, 2);
                setState(260);
                match(xCEPParser::INT);
                break;
            }

            case xCEPParser::NAME: {
                enterOuterAlt(_localctx, 3);
                setState(261);
                match(xCEPParser::NAME);
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ExpressionsContext ------------------------------------------------------------------

xCEPParser::ExpressionsContext::ExpressionsContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<xCEPParser::ExpressionContext*> xCEPParser::ExpressionsContext::expression() {
    return getRuleContexts<xCEPParser::ExpressionContext>();
}

xCEPParser::ExpressionContext* xCEPParser::ExpressionsContext::expression(size_t i) {
    return getRuleContext<xCEPParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode*> xCEPParser::ExpressionsContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::ExpressionsContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

size_t xCEPParser::ExpressionsContext::getRuleIndex() const { return xCEPParser::RuleExpressions; }

void xCEPParser::ExpressionsContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterExpressions(this);
}

void xCEPParser::ExpressionsContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitExpressions(this);
}

xCEPParser::ExpressionsContext* xCEPParser::expressions() {
    ExpressionsContext* _localctx = _tracker.createInstance<ExpressionsContext>(_ctx, getState());
    enterRule(_localctx, 54, xCEPParser::RuleExpressions);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(264);
        expression(0);
        setState(269);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == xCEPParser::COMMA) {
            setState(265);
            match(xCEPParser::COMMA);
            setState(266);
            expression(0);
            setState(271);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

xCEPParser::ExpressionContext::ExpressionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t xCEPParser::ExpressionContext::getRuleIndex() const { return xCEPParser::RuleExpression; }

void xCEPParser::ExpressionContext::copyFrom(ExpressionContext* ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- IsExpressionContext ------------------------------------------------------------------

xCEPParser::PredicateContext* xCEPParser::IsExpressionContext::predicate() {
    return getRuleContext<xCEPParser::PredicateContext>(0);
}

tree::TerminalNode* xCEPParser::IsExpressionContext::IS() { return getToken(xCEPParser::IS, 0); }

tree::TerminalNode* xCEPParser::IsExpressionContext::TRUE() { return getToken(xCEPParser::TRUE, 0); }

tree::TerminalNode* xCEPParser::IsExpressionContext::FALSE() { return getToken(xCEPParser::FALSE, 0); }

tree::TerminalNode* xCEPParser::IsExpressionContext::UNKNOWN() { return getToken(xCEPParser::UNKNOWN, 0); }

tree::TerminalNode* xCEPParser::IsExpressionContext::NOT() { return getToken(xCEPParser::NOT, 0); }

xCEPParser::IsExpressionContext::IsExpressionContext(ExpressionContext* ctx) { copyFrom(ctx); }

void xCEPParser::IsExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterIsExpression(this);
}
void xCEPParser::IsExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitIsExpression(this);
}
//----------------- NotExpressionContext ------------------------------------------------------------------

tree::TerminalNode* xCEPParser::NotExpressionContext::NOT_OP() { return getToken(xCEPParser::NOT_OP, 0); }

xCEPParser::ExpressionContext* xCEPParser::NotExpressionContext::expression() {
    return getRuleContext<xCEPParser::ExpressionContext>(0);
}

xCEPParser::NotExpressionContext::NotExpressionContext(ExpressionContext* ctx) { copyFrom(ctx); }

void xCEPParser::NotExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterNotExpression(this);
}
void xCEPParser::NotExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitNotExpression(this);
}
//----------------- LogicalExpressionContext ------------------------------------------------------------------

std::vector<xCEPParser::ExpressionContext*> xCEPParser::LogicalExpressionContext::expression() {
    return getRuleContexts<xCEPParser::ExpressionContext>();
}

xCEPParser::ExpressionContext* xCEPParser::LogicalExpressionContext::expression(size_t i) {
    return getRuleContext<xCEPParser::ExpressionContext>(i);
}

xCEPParser::LogicalOperatorContext* xCEPParser::LogicalExpressionContext::logicalOperator() {
    return getRuleContext<xCEPParser::LogicalOperatorContext>(0);
}

xCEPParser::LogicalExpressionContext::LogicalExpressionContext(ExpressionContext* ctx) { copyFrom(ctx); }

void xCEPParser::LogicalExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterLogicalExpression(this);
}
void xCEPParser::LogicalExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitLogicalExpression(this);
}
//----------------- PredicateExpressionContext ------------------------------------------------------------------

xCEPParser::PredicateContext* xCEPParser::PredicateExpressionContext::predicate() {
    return getRuleContext<xCEPParser::PredicateContext>(0);
}

xCEPParser::PredicateExpressionContext::PredicateExpressionContext(ExpressionContext* ctx) { copyFrom(ctx); }

void xCEPParser::PredicateExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterPredicateExpression(this);
}
void xCEPParser::PredicateExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitPredicateExpression(this);
}

xCEPParser::ExpressionContext* xCEPParser::expression() { return expression(0); }

xCEPParser::ExpressionContext* xCEPParser::expression(int precedence) {
    ParserRuleContext* parentContext = _ctx;
    size_t parentState = getState();
    xCEPParser::ExpressionContext* _localctx = _tracker.createInstance<ExpressionContext>(_ctx, parentState);
    xCEPParser::ExpressionContext* previousContext = _localctx;
    (void) previousContext;// Silence compiler, in case the context is not used by generated code.
    size_t startState = 56;
    enterRecursionRule(_localctx, 56, xCEPParser::RuleExpression, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        setState(283);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 26, _ctx)) {
            case 1: {
                _localctx = _tracker.createInstance<NotExpressionContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;

                setState(273);
                match(xCEPParser::NOT_OP);
                setState(274);
                expression(4);
                break;
            }

            case 2: {
                _localctx = _tracker.createInstance<IsExpressionContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(275);
                predicate(0);
                setState(276);
                match(xCEPParser::IS);
                setState(278);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::NOT) {
                    setState(277);
                    match(xCEPParser::NOT);
                }
                setState(280);
                dynamic_cast<IsExpressionContext*>(_localctx)->testValue = _input->LT(1);
                _la = _input->LA(1);
                if (!((((_la & ~0x3fULL) == 0)
                       && ((1ULL << _la)
                           & ((1ULL << xCEPParser::TRUE) | (1ULL << xCEPParser::FALSE) | (1ULL << xCEPParser::UNKNOWN)))
                           != 0))) {
                    dynamic_cast<IsExpressionContext*>(_localctx)->testValue = _errHandler->recoverInline(this);
                } else {
                    _errHandler->reportMatch(this);
                    consume();
                }
                break;
            }

            case 3: {
                _localctx = _tracker.createInstance<PredicateExpressionContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(282);
                predicate(0);
                break;
            }

            default: break;
        }
        _ctx->stop = _input->LT(-1);
        setState(291);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<LogicalExpressionContext>(
                    _tracker.createInstance<ExpressionContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleExpression);
                setState(285);

                if (!(precpred(_ctx, 3)))
                    throw FailedPredicateException(this, "precpred(_ctx, 3)");
                setState(286);
                logicalOperator();
                setState(287);
                expression(4);
            }
            setState(293);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
        }
    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- PredicateContext ------------------------------------------------------------------

xCEPParser::PredicateContext::PredicateContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t xCEPParser::PredicateContext::getRuleIndex() const { return xCEPParser::RulePredicate; }

void xCEPParser::PredicateContext::copyFrom(PredicateContext* ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- ExpressionAtomPredicateContext ------------------------------------------------------------------

xCEPParser::ExpressionAtomContext* xCEPParser::ExpressionAtomPredicateContext::expressionAtom() {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(0);
}

xCEPParser::ExpressionAtomPredicateContext::ExpressionAtomPredicateContext(PredicateContext* ctx) { copyFrom(ctx); }

void xCEPParser::ExpressionAtomPredicateContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterExpressionAtomPredicate(this);
}
void xCEPParser::ExpressionAtomPredicateContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitExpressionAtomPredicate(this);
}
//----------------- InPredicateContext ------------------------------------------------------------------

xCEPParser::PredicateContext* xCEPParser::InPredicateContext::predicate() {
    return getRuleContext<xCEPParser::PredicateContext>(0);
}

tree::TerminalNode* xCEPParser::InPredicateContext::IN() { return getToken(xCEPParser::IN, 0); }

tree::TerminalNode* xCEPParser::InPredicateContext::LPARENTHESIS() { return getToken(xCEPParser::LPARENTHESIS, 0); }

xCEPParser::ExpressionsContext* xCEPParser::InPredicateContext::expressions() {
    return getRuleContext<xCEPParser::ExpressionsContext>(0);
}

tree::TerminalNode* xCEPParser::InPredicateContext::RPARENTHESIS() { return getToken(xCEPParser::RPARENTHESIS, 0); }

tree::TerminalNode* xCEPParser::InPredicateContext::NOT() { return getToken(xCEPParser::NOT, 0); }

xCEPParser::InPredicateContext::InPredicateContext(PredicateContext* ctx) { copyFrom(ctx); }

void xCEPParser::InPredicateContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterInPredicate(this);
}
void xCEPParser::InPredicateContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitInPredicate(this);
}
//----------------- BinaryComparasionPredicateContext ------------------------------------------------------------------

xCEPParser::ComparisonOperatorContext* xCEPParser::BinaryComparasionPredicateContext::comparisonOperator() {
    return getRuleContext<xCEPParser::ComparisonOperatorContext>(0);
}

std::vector<xCEPParser::PredicateContext*> xCEPParser::BinaryComparasionPredicateContext::predicate() {
    return getRuleContexts<xCEPParser::PredicateContext>();
}

xCEPParser::PredicateContext* xCEPParser::BinaryComparasionPredicateContext::predicate(size_t i) {
    return getRuleContext<xCEPParser::PredicateContext>(i);
}

xCEPParser::BinaryComparasionPredicateContext::BinaryComparasionPredicateContext(PredicateContext* ctx) { copyFrom(ctx); }

void xCEPParser::BinaryComparasionPredicateContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterBinaryComparisonPredicate(this);
}
void xCEPParser::BinaryComparasionPredicateContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitBinaryComparisonPredicate(this);
}
//----------------- IsNullPredicateContext ------------------------------------------------------------------

xCEPParser::PredicateContext* xCEPParser::IsNullPredicateContext::predicate() {
    return getRuleContext<xCEPParser::PredicateContext>(0);
}

tree::TerminalNode* xCEPParser::IsNullPredicateContext::IS() { return getToken(xCEPParser::IS, 0); }

xCEPParser::NullNotnullContext* xCEPParser::IsNullPredicateContext::nullNotnull() {
    return getRuleContext<xCEPParser::NullNotnullContext>(0);
}

xCEPParser::IsNullPredicateContext::IsNullPredicateContext(PredicateContext* ctx) { copyFrom(ctx); }

void xCEPParser::IsNullPredicateContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterIsNullPredicate(this);
}
void xCEPParser::IsNullPredicateContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitIsNullPredicate(this);
}

xCEPParser::PredicateContext* xCEPParser::predicate() { return predicate(0); }

xCEPParser::PredicateContext* xCEPParser::predicate(int precedence) {
    ParserRuleContext* parentContext = _ctx;
    size_t parentState = getState();
    xCEPParser::PredicateContext* _localctx = _tracker.createInstance<PredicateContext>(_ctx, parentState);
    xCEPParser::PredicateContext* previousContext = _localctx;
    (void) previousContext;// Silence compiler, in case the context is not used by generated code.
    size_t startState = 58;
    enterRecursionRule(_localctx, 58, xCEPParser::RulePredicate, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<ExpressionAtomPredicateContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(295);
        expressionAtom(0);
        _ctx->stop = _input->LT(-1);
        setState(315);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                setState(313);
                _errHandler->sync(this);
                switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx)) {
                    case 1: {
                        auto newContext = _tracker.createInstance<BinaryComparasionPredicateContext>(
                            _tracker.createInstance<PredicateContext>(parentContext, parentState));
                        _localctx = newContext;
                        newContext->left = previousContext;
                        pushNewRecursionContext(newContext, startState, RulePredicate);
                        setState(297);

                        if (!(precpred(_ctx, 2)))
                            throw FailedPredicateException(this, "precpred(_ctx, 2)");
                        setState(298);
                        comparisonOperator();
                        setState(299);
                        dynamic_cast<BinaryComparasionPredicateContext*>(_localctx)->right = predicate(3);
                        break;
                    }

                    case 2: {
                        auto newContext = _tracker.createInstance<InPredicateContext>(
                            _tracker.createInstance<PredicateContext>(parentContext, parentState));
                        _localctx = newContext;
                        pushNewRecursionContext(newContext, startState, RulePredicate);
                        setState(301);

                        if (!(precpred(_ctx, 4)))
                            throw FailedPredicateException(this, "precpred(_ctx, 4)");
                        setState(303);
                        _errHandler->sync(this);

                        _la = _input->LA(1);
                        if (_la == xCEPParser::NOT) {
                            setState(302);
                            match(xCEPParser::NOT);
                        }
                        setState(305);
                        match(xCEPParser::IN);
                        setState(306);
                        match(xCEPParser::LPARENTHESIS);
                        setState(307);
                        expressions();
                        setState(308);
                        match(xCEPParser::RPARENTHESIS);
                        break;
                    }

                    case 3: {
                        auto newContext = _tracker.createInstance<IsNullPredicateContext>(
                            _tracker.createInstance<PredicateContext>(parentContext, parentState));
                        _localctx = newContext;
                        pushNewRecursionContext(newContext, startState, RulePredicate);
                        setState(310);

                        if (!(precpred(_ctx, 3)))
                            throw FailedPredicateException(this, "precpred(_ctx, 3)");
                        setState(311);
                        match(xCEPParser::IS);
                        setState(312);
                        nullNotnull();
                        break;
                    }

                    default: break;
                }
            }
            setState(317);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
        }
    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- ExpressionAtomContext ------------------------------------------------------------------

xCEPParser::ExpressionAtomContext::ExpressionAtomContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t xCEPParser::ExpressionAtomContext::getRuleIndex() const { return xCEPParser::RuleExpressionAtom; }

void xCEPParser::ExpressionAtomContext::copyFrom(ExpressionAtomContext* ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- UnaryExpressionAtomContext ------------------------------------------------------------------

xCEPParser::UnaryOperatorContext* xCEPParser::UnaryExpressionAtomContext::unaryOperator() {
    return getRuleContext<xCEPParser::UnaryOperatorContext>(0);
}

xCEPParser::ExpressionAtomContext* xCEPParser::UnaryExpressionAtomContext::expressionAtom() {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(0);
}

xCEPParser::UnaryExpressionAtomContext::UnaryExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::UnaryExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterUnaryExpressionAtom(this);
}
void xCEPParser::UnaryExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitUnaryExpressionAtom(this);
}
//----------------- AttributeAtomContext ------------------------------------------------------------------

xCEPParser::EventAttributeContext* xCEPParser::AttributeAtomContext::eventAttribute() {
    return getRuleContext<xCEPParser::EventAttributeContext>(0);
}

xCEPParser::AttributeAtomContext::AttributeAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::AttributeAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterAttributeAtom(this);
}
void xCEPParser::AttributeAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitAttributeAtom(this);
}
//----------------- ConstantExpressionAtomContext ------------------------------------------------------------------

xCEPParser::ConstantContext* xCEPParser::ConstantExpressionAtomContext::constant() {
    return getRuleContext<xCEPParser::ConstantContext>(0);
}

xCEPParser::ConstantExpressionAtomContext::ConstantExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::ConstantExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterConstantExpressionAtom(this);
}
void xCEPParser::ConstantExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitConstantExpressionAtom(this);
}
//----------------- BinaryExpressionAtomContext ------------------------------------------------------------------

tree::TerminalNode* xCEPParser::BinaryExpressionAtomContext::BINARY() { return getToken(xCEPParser::BINARY, 0); }

xCEPParser::ExpressionAtomContext* xCEPParser::BinaryExpressionAtomContext::expressionAtom() {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(0);
}

xCEPParser::BinaryExpressionAtomContext::BinaryExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::BinaryExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterBinaryExpressionAtom(this);
}
void xCEPParser::BinaryExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitBinaryExpressionAtom(this);
}
//----------------- BitExpressionAtomContext ------------------------------------------------------------------

xCEPParser::BitOperatorContext* xCEPParser::BitExpressionAtomContext::bitOperator() {
    return getRuleContext<xCEPParser::BitOperatorContext>(0);
}

std::vector<xCEPParser::ExpressionAtomContext*> xCEPParser::BitExpressionAtomContext::expressionAtom() {
    return getRuleContexts<xCEPParser::ExpressionAtomContext>();
}

xCEPParser::ExpressionAtomContext* xCEPParser::BitExpressionAtomContext::expressionAtom(size_t i) {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(i);
}

xCEPParser::BitExpressionAtomContext::BitExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::BitExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterBitExpressionAtom(this);
}
void xCEPParser::BitExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitBitExpressionAtom(this);
}
//----------------- xtedExpressionAtomContext ------------------------------------------------------------------

tree::TerminalNode* xCEPParser::xtedExpressionAtomContext::LPARENTHESIS() { return getToken(xCEPParser::LPARENTHESIS, 0); }

std::vector<xCEPParser::ExpressionContext*> xCEPParser::xtedExpressionAtomContext::expression() {
    return getRuleContexts<xCEPParser::ExpressionContext>();
}

xCEPParser::ExpressionContext* xCEPParser::xtedExpressionAtomContext::expression(size_t i) {
    return getRuleContext<xCEPParser::ExpressionContext>(i);
}

tree::TerminalNode* xCEPParser::xtedExpressionAtomContext::RPARENTHESIS() { return getToken(xCEPParser::RPARENTHESIS, 0); }

std::vector<tree::TerminalNode*> xCEPParser::xtedExpressionAtomContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::xtedExpressionAtomContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

xCEPParser::xtedExpressionAtomContext::xtedExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::xtedExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterxtedExpressionAtom(this);
}
void xCEPParser::xtedExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitxtedExpressionAtom(this);
}
//----------------- MathExpressionAtomContext ------------------------------------------------------------------

xCEPParser::MathOperatorContext* xCEPParser::MathExpressionAtomContext::mathOperator() {
    return getRuleContext<xCEPParser::MathOperatorContext>(0);
}

std::vector<xCEPParser::ExpressionAtomContext*> xCEPParser::MathExpressionAtomContext::expressionAtom() {
    return getRuleContexts<xCEPParser::ExpressionAtomContext>();
}

xCEPParser::ExpressionAtomContext* xCEPParser::MathExpressionAtomContext::expressionAtom(size_t i) {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(i);
}

xCEPParser::MathExpressionAtomContext::MathExpressionAtomContext(ExpressionAtomContext* ctx) { copyFrom(ctx); }

void xCEPParser::MathExpressionAtomContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterMathExpressionAtom(this);
}
void xCEPParser::MathExpressionAtomContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitMathExpressionAtom(this);
}

xCEPParser::ExpressionAtomContext* xCEPParser::expressionAtom() { return expressionAtom(0); }

xCEPParser::ExpressionAtomContext* xCEPParser::expressionAtom(int precedence) {
    ParserRuleContext* parentContext = _ctx;
    size_t parentState = getState();
    xCEPParser::ExpressionAtomContext* _localctx = _tracker.createInstance<ExpressionAtomContext>(_ctx, parentState);
    xCEPParser::ExpressionAtomContext* previousContext = _localctx;
    (void) previousContext;// Silence compiler, in case the context is not used by generated code.
    size_t startState = 60;
    enterRecursionRule(_localctx, 60, xCEPParser::RuleExpressionAtom, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        setState(337);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx)) {
            case 1: {
                _localctx = _tracker.createInstance<AttributeAtomContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;

                setState(319);
                eventAttribute();
                break;
            }

            case 2: {
                _localctx = _tracker.createInstance<UnaryExpressionAtomContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(320);
                unaryOperator();
                setState(321);
                expressionAtom(6);
                break;
            }

            case 3: {
                _localctx = _tracker.createInstance<BinaryExpressionAtomContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(323);
                match(xCEPParser::BINARY);
                setState(324);
                expressionAtom(5);
                break;
            }

            case 4: {
                _localctx = _tracker.createInstance<xtedExpressionAtomContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(325);
                match(xCEPParser::LPARENTHESIS);
                setState(326);
                expression(0);
                setState(331);
                _errHandler->sync(this);
                _la = _input->LA(1);
                while (_la == xCEPParser::COMMA) {
                    setState(327);
                    match(xCEPParser::COMMA);
                    setState(328);
                    expression(0);
                    setState(333);
                    _errHandler->sync(this);
                    _la = _input->LA(1);
                }
                setState(334);
                match(xCEPParser::RPARENTHESIS);
                break;
            }

            case 5: {
                _localctx = _tracker.createInstance<ConstantExpressionAtomContext>(_localctx);
                _ctx = _localctx;
                previousContext = _localctx;
                setState(336);
                constant();
                break;
            }

            default: break;
        }
        _ctx->stop = _input->LT(-1);
        setState(349);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                setState(347);
                _errHandler->sync(this);
                switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx)) {
                    case 1: {
                        auto newContext = _tracker.createInstance<BitExpressionAtomContext>(
                            _tracker.createInstance<ExpressionAtomContext>(parentContext, parentState));
                        _localctx = newContext;
                        newContext->left = previousContext;
                        pushNewRecursionContext(newContext, startState, RuleExpressionAtom);
                        setState(339);

                        if (!(precpred(_ctx, 3)))
                            throw FailedPredicateException(this, "precpred(_ctx, 3)");
                        setState(340);
                        bitOperator();
                        setState(341);
                        dynamic_cast<BitExpressionAtomContext*>(_localctx)->right = expressionAtom(4);
                        break;
                    }

                    case 2: {
                        auto newContext = _tracker.createInstance<MathExpressionAtomContext>(
                            _tracker.createInstance<ExpressionAtomContext>(parentContext, parentState));
                        _localctx = newContext;
                        newContext->left = previousContext;
                        pushNewRecursionContext(newContext, startState, RuleExpressionAtom);
                        setState(343);

                        if (!(precpred(_ctx, 2)))
                            throw FailedPredicateException(this, "precpred(_ctx, 2)");
                        setState(344);
                        mathOperator();
                        setState(345);
                        dynamic_cast<MathExpressionAtomContext*>(_localctx)->right = expressionAtom(3);
                        break;
                    }

                    default: break;
                }
            }
            setState(351);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
        }
    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- EventAttributeContext ------------------------------------------------------------------

xCEPParser::EventAttributeContext::EventAttributeContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

xCEPParser::AggregationContext* xCEPParser::EventAttributeContext::aggregation() {
    return getRuleContext<xCEPParser::AggregationContext>(0);
}

tree::TerminalNode* xCEPParser::EventAttributeContext::LPARENTHESIS() { return getToken(xCEPParser::LPARENTHESIS, 0); }

xCEPParser::ExpressionsContext* xCEPParser::EventAttributeContext::expressions() {
    return getRuleContext<xCEPParser::ExpressionsContext>(0);
}

tree::TerminalNode* xCEPParser::EventAttributeContext::RPARENTHESIS() { return getToken(xCEPParser::RPARENTHESIS, 0); }

xCEPParser::EventIterationContext* xCEPParser::EventAttributeContext::eventIteration() {
    return getRuleContext<xCEPParser::EventIterationContext>(0);
}

tree::TerminalNode* xCEPParser::EventAttributeContext::POINT() { return getToken(xCEPParser::POINT, 0); }

xCEPParser::AttributeContext* xCEPParser::EventAttributeContext::attribute() {
    return getRuleContext<xCEPParser::AttributeContext>(0);
}

tree::TerminalNode* xCEPParser::EventAttributeContext::NAME() { return getToken(xCEPParser::NAME, 0); }

size_t xCEPParser::EventAttributeContext::getRuleIndex() const { return xCEPParser::RuleEventAttribute; }

void xCEPParser::EventAttributeContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterEventAttribute(this);
}

void xCEPParser::EventAttributeContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitEventAttribute(this);
}

xCEPParser::EventAttributeContext* xCEPParser::eventAttribute() {
    EventAttributeContext* _localctx = _tracker.createInstance<EventAttributeContext>(_ctx, getState());
    enterRule(_localctx, 62, xCEPParser::RuleEventAttribute);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(365);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(352);
                aggregation();
                setState(353);
                match(xCEPParser::LPARENTHESIS);
                setState(354);
                expressions();
                setState(355);
                match(xCEPParser::RPARENTHESIS);
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(357);
                eventIteration();
                setState(360);
                _errHandler->sync(this);

                switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx)) {
                    case 1: {
                        setState(358);
                        match(xCEPParser::POINT);
                        setState(359);
                        attribute();
                        break;
                    }

                    default: break;
                }
                break;
            }

            case 3: {
                enterOuterAlt(_localctx, 3);
                setState(362);
                match(xCEPParser::NAME);
                setState(363);
                match(xCEPParser::POINT);
                setState(364);
                attribute();
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- EventIterationContext ------------------------------------------------------------------

xCEPParser::EventIterationContext::EventIterationContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::EventIterationContext::NAME() { return getToken(xCEPParser::NAME, 0); }

tree::TerminalNode* xCEPParser::EventIterationContext::LBRACKET() { return getToken(xCEPParser::LBRACKET, 0); }

tree::TerminalNode* xCEPParser::EventIterationContext::RBRACKET() { return getToken(xCEPParser::RBRACKET, 0); }

xCEPParser::MathExpressionContext* xCEPParser::EventIterationContext::mathExpression() {
    return getRuleContext<xCEPParser::MathExpressionContext>(0);
}

size_t xCEPParser::EventIterationContext::getRuleIndex() const { return xCEPParser::RuleEventIteration; }

void xCEPParser::EventIterationContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterEventIteration(this);
}

void xCEPParser::EventIterationContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitEventIteration(this);
}

xCEPParser::EventIterationContext* xCEPParser::eventIteration() {
    EventIterationContext* _localctx = _tracker.createInstance<EventIterationContext>(_ctx, getState());
    enterRule(_localctx, 64, xCEPParser::RuleEventIteration);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(367);
        match(xCEPParser::NAME);
        setState(368);
        match(xCEPParser::LBRACKET);
        setState(370);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~0x3fULL) == 0)
             && ((1ULL << _la)
                 & ((1ULL << xCEPParser::T__0) | (1ULL << xCEPParser::LPARENTHESIS) | (1ULL << xCEPParser::NOT)
                    | (1ULL << xCEPParser::PLUS) | (1ULL << xCEPParser::BINARY) | (1ULL << xCEPParser::QUOTE)))
                 != 0)
            || ((((_la - 64) & ~0x3fULL) == 0)
                && ((1ULL << (_la - 64))
                    & ((1ULL << (xCEPParser::AVG - 64)) | (1ULL << (xCEPParser::SUM - 64))
                       | (1ULL << (xCEPParser::MIN - 64)) | (1ULL << (xCEPParser::MAX - 64))
                       | (1ULL << (xCEPParser::COUNT - 64)) | (1ULL << (xCEPParser::INT - 64))
                       | (1ULL << (xCEPParser::NAME - 64))))
                    != 0)) {
            setState(369);
            mathExpression();
        }
        setState(372);
        match(xCEPParser::RBRACKET);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- MathExpressionContext ------------------------------------------------------------------

xCEPParser::MathExpressionContext::MathExpressionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

xCEPParser::MathOperatorContext* xCEPParser::MathExpressionContext::mathOperator() {
    return getRuleContext<xCEPParser::MathOperatorContext>(0);
}

std::vector<xCEPParser::ExpressionAtomContext*> xCEPParser::MathExpressionContext::expressionAtom() {
    return getRuleContexts<xCEPParser::ExpressionAtomContext>();
}

xCEPParser::ExpressionAtomContext* xCEPParser::MathExpressionContext::expressionAtom(size_t i) {
    return getRuleContext<xCEPParser::ExpressionAtomContext>(i);
}

std::vector<xCEPParser::ConstantContext*> xCEPParser::MathExpressionContext::constant() {
    return getRuleContexts<xCEPParser::ConstantContext>();
}

xCEPParser::ConstantContext* xCEPParser::MathExpressionContext::constant(size_t i) {
    return getRuleContext<xCEPParser::ConstantContext>(i);
}

tree::TerminalNode* xCEPParser::MathExpressionContext::D_POINTS() { return getToken(xCEPParser::D_POINTS, 0); }

size_t xCEPParser::MathExpressionContext::getRuleIndex() const { return xCEPParser::RuleMathExpression; }

void xCEPParser::MathExpressionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterMathExpression(this);
}

void xCEPParser::MathExpressionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitMathExpression(this);
}

xCEPParser::MathExpressionContext* xCEPParser::mathExpression() {
    MathExpressionContext* _localctx = _tracker.createInstance<MathExpressionContext>(_ctx, getState());
    enterRule(_localctx, 66, xCEPParser::RuleMathExpression);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(383);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 39, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(374);
                dynamic_cast<MathExpressionContext*>(_localctx)->left = expressionAtom(0);
                setState(375);
                mathOperator();
                setState(376);
                dynamic_cast<MathExpressionContext*>(_localctx)->right = expressionAtom(0);
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(378);
                constant();
                setState(381);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if (_la == xCEPParser::D_POINTS) {
                    setState(379);
                    match(xCEPParser::D_POINTS);
                    setState(380);
                    constant();
                }
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- AggregationContext ------------------------------------------------------------------

xCEPParser::AggregationContext::AggregationContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::AggregationContext::AVG() { return getToken(xCEPParser::AVG, 0); }

tree::TerminalNode* xCEPParser::AggregationContext::SUM() { return getToken(xCEPParser::SUM, 0); }

tree::TerminalNode* xCEPParser::AggregationContext::MIN() { return getToken(xCEPParser::MIN, 0); }

tree::TerminalNode* xCEPParser::AggregationContext::MAX() { return getToken(xCEPParser::MAX, 0); }

tree::TerminalNode* xCEPParser::AggregationContext::COUNT() { return getToken(xCEPParser::COUNT, 0); }

size_t xCEPParser::AggregationContext::getRuleIndex() const { return xCEPParser::RuleAggregation; }

void xCEPParser::AggregationContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterAggregation(this);
}

void xCEPParser::AggregationContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitAggregation(this);
}

xCEPParser::AggregationContext* xCEPParser::aggregation() {
    AggregationContext* _localctx = _tracker.createInstance<AggregationContext>(_ctx, getState());
    enterRule(_localctx, 68, xCEPParser::RuleAggregation);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(391);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::LPARENTHESIS: {
                enterOuterAlt(_localctx, 1);

                break;
            }

            case xCEPParser::AVG: {
                enterOuterAlt(_localctx, 2);
                setState(386);
                match(xCEPParser::AVG);
                break;
            }

            case xCEPParser::SUM: {
                enterOuterAlt(_localctx, 3);
                setState(387);
                match(xCEPParser::SUM);
                break;
            }

            case xCEPParser::MIN: {
                enterOuterAlt(_localctx, 4);
                setState(388);
                match(xCEPParser::MIN);
                break;
            }

            case xCEPParser::MAX: {
                enterOuterAlt(_localctx, 5);
                setState(389);
                match(xCEPParser::MAX);
                break;
            }

            case xCEPParser::COUNT: {
                enterOuterAlt(_localctx, 6);
                setState(390);
                match(xCEPParser::COUNT);
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- AttributeContext ------------------------------------------------------------------

xCEPParser::AttributeContext::AttributeContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::AttributeContext::NAME() { return getToken(xCEPParser::NAME, 0); }

size_t xCEPParser::AttributeContext::getRuleIndex() const { return xCEPParser::RuleAttribute; }

void xCEPParser::AttributeContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterAttribute(this);
}

void xCEPParser::AttributeContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitAttribute(this);
}

xCEPParser::AttributeContext* xCEPParser::attribute() {
    AttributeContext* _localctx = _tracker.createInstance<AttributeContext>(_ctx, getState());
    enterRule(_localctx, 70, xCEPParser::RuleAttribute);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(393);
        match(xCEPParser::NAME);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- AttValContext ------------------------------------------------------------------

xCEPParser::AttValContext::AttValContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::AttValContext::IF() { return getToken(xCEPParser::IF, 0); }

xCEPParser::ConditionContext* xCEPParser::AttValContext::condition() {
    return getRuleContext<xCEPParser::ConditionContext>(0);
}

xCEPParser::EventAttributeContext* xCEPParser::AttValContext::eventAttribute() {
    return getRuleContext<xCEPParser::EventAttributeContext>(0);
}

xCEPParser::EventContext* xCEPParser::AttValContext::event() { return getRuleContext<xCEPParser::EventContext>(0); }

xCEPParser::ExpressionContext* xCEPParser::AttValContext::expression() {
    return getRuleContext<xCEPParser::ExpressionContext>(0);
}

xCEPParser::BoolRuleContext* xCEPParser::AttValContext::boolRule() {
    return getRuleContext<xCEPParser::BoolRuleContext>(0);
}

size_t xCEPParser::AttValContext::getRuleIndex() const { return xCEPParser::RuleAttVal; }

void xCEPParser::AttValContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterAttVal(this);
}

void xCEPParser::AttValContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitAttVal(this);
}

xCEPParser::AttValContext* xCEPParser::attVal() {
    AttValContext* _localctx = _tracker.createInstance<AttValContext>(_ctx, getState());
    enterRule(_localctx, 72, xCEPParser::RuleAttVal);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(401);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(395);
                match(xCEPParser::IF);
                setState(396);
                condition();
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(397);
                eventAttribute();
                break;
            }

            case 3: {
                enterOuterAlt(_localctx, 3);
                setState(398);
                event();
                break;
            }

            case 4: {
                enterOuterAlt(_localctx, 4);
                setState(399);
                expression(0);
                break;
            }

            case 5: {
                enterOuterAlt(_localctx, 5);
                setState(400);
                boolRule();
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- BoolRuleContext ------------------------------------------------------------------

xCEPParser::BoolRuleContext::BoolRuleContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::BoolRuleContext::TRUE() { return getToken(xCEPParser::TRUE, 0); }

tree::TerminalNode* xCEPParser::BoolRuleContext::FALSE() { return getToken(xCEPParser::FALSE, 0); }

size_t xCEPParser::BoolRuleContext::getRuleIndex() const { return xCEPParser::RuleBoolRule; }

void xCEPParser::BoolRuleContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterBoolRule(this);
}

void xCEPParser::BoolRuleContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitBoolRule(this);
}

xCEPParser::BoolRuleContext* xCEPParser::boolRule() {
    BoolRuleContext* _localctx = _tracker.createInstance<BoolRuleContext>(_ctx, getState());
    enterRule(_localctx, 74, xCEPParser::RuleBoolRule);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(403);
        _la = _input->LA(1);
        if (!(_la == xCEPParser::TRUE

              || _la == xCEPParser::FALSE)) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConditionContext ------------------------------------------------------------------

xCEPParser::ConditionContext::ConditionContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::ConditionContext::LPARENTHESIS() { return getToken(xCEPParser::LPARENTHESIS, 0); }

xCEPParser::ExpressionContext* xCEPParser::ConditionContext::expression() {
    return getRuleContext<xCEPParser::ExpressionContext>(0);
}

std::vector<tree::TerminalNode*> xCEPParser::ConditionContext::COMMA() { return getTokens(xCEPParser::COMMA); }

tree::TerminalNode* xCEPParser::ConditionContext::COMMA(size_t i) { return getToken(xCEPParser::COMMA, i); }

std::vector<xCEPParser::AttValContext*> xCEPParser::ConditionContext::attVal() {
    return getRuleContexts<xCEPParser::AttValContext>();
}

xCEPParser::AttValContext* xCEPParser::ConditionContext::attVal(size_t i) {
    return getRuleContext<xCEPParser::AttValContext>(i);
}

tree::TerminalNode* xCEPParser::ConditionContext::RPARENTHESIS() { return getToken(xCEPParser::RPARENTHESIS, 0); }

size_t xCEPParser::ConditionContext::getRuleIndex() const { return xCEPParser::RuleCondition; }

void xCEPParser::ConditionContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterCondition(this);
}

void xCEPParser::ConditionContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitCondition(this);
}

xCEPParser::ConditionContext* xCEPParser::condition() {
    ConditionContext* _localctx = _tracker.createInstance<ConditionContext>(_ctx, getState());
    enterRule(_localctx, 76, xCEPParser::RuleCondition);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(405);
        match(xCEPParser::LPARENTHESIS);
        setState(406);
        expression(0);
        setState(407);
        match(xCEPParser::COMMA);
        setState(408);
        attVal();
        setState(409);
        match(xCEPParser::COMMA);
        setState(410);
        attVal();
        setState(411);
        match(xCEPParser::RPARENTHESIS);

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- UnaryOperatorContext ------------------------------------------------------------------

xCEPParser::UnaryOperatorContext::UnaryOperatorContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::UnaryOperatorContext::PLUS() { return getToken(xCEPParser::PLUS, 0); }

tree::TerminalNode* xCEPParser::UnaryOperatorContext::NOT() { return getToken(xCEPParser::NOT, 0); }

size_t xCEPParser::UnaryOperatorContext::getRuleIndex() const { return xCEPParser::RuleUnaryOperator; }

void xCEPParser::UnaryOperatorContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterUnaryOperator(this);
}

void xCEPParser::UnaryOperatorContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitUnaryOperator(this);
}

xCEPParser::UnaryOperatorContext* xCEPParser::unaryOperator() {
    UnaryOperatorContext* _localctx = _tracker.createInstance<UnaryOperatorContext>(_ctx, getState());
    enterRule(_localctx, 78, xCEPParser::RuleUnaryOperator);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(413);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0)
               && ((1ULL << _la) & ((1ULL << xCEPParser::T__0) | (1ULL << xCEPParser::NOT) | (1ULL << xCEPParser::PLUS)))
                   != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ComparisonOperatorContext ------------------------------------------------------------------

xCEPParser::ComparisonOperatorContext::ComparisonOperatorContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<tree::TerminalNode*> xCEPParser::ComparisonOperatorContext::EQUAL() { return getTokens(xCEPParser::EQUAL); }

tree::TerminalNode* xCEPParser::ComparisonOperatorContext::EQUAL(size_t i) { return getToken(xCEPParser::EQUAL, i); }

tree::TerminalNode* xCEPParser::ComparisonOperatorContext::NOT_OP() { return getToken(xCEPParser::NOT_OP, 0); }

size_t xCEPParser::ComparisonOperatorContext::getRuleIndex() const { return xCEPParser::RuleComparisonOperator; }

void xCEPParser::ComparisonOperatorContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterComparisonOperator(this);
}

void xCEPParser::ComparisonOperatorContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitComparisonOperator(this);
}

xCEPParser::ComparisonOperatorContext* xCEPParser::comparisonOperator() {
    ComparisonOperatorContext* _localctx = _tracker.createInstance<ComparisonOperatorContext>(_ctx, getState());
    enterRule(_localctx, 80, xCEPParser::RuleComparisonOperator);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(431);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 42, _ctx)) {
            case 1: {
                enterOuterAlt(_localctx, 1);
                setState(415);
                match(xCEPParser::EQUAL);
                setState(416);
                match(xCEPParser::EQUAL);
                break;
            }

            case 2: {
                enterOuterAlt(_localctx, 2);
                setState(417);
                match(xCEPParser::T__1);
                break;
            }

            case 3: {
                enterOuterAlt(_localctx, 3);
                setState(418);
                match(xCEPParser::T__2);
                break;
            }

            case 4: {
                enterOuterAlt(_localctx, 4);
                setState(419);
                match(xCEPParser::T__2);
                setState(420);
                match(xCEPParser::EQUAL);
                break;
            }

            case 5: {
                enterOuterAlt(_localctx, 5);
                setState(421);
                match(xCEPParser::T__1);
                setState(422);
                match(xCEPParser::EQUAL);
                break;
            }

            case 6: {
                enterOuterAlt(_localctx, 6);
                setState(423);
                match(xCEPParser::T__2);
                setState(424);
                match(xCEPParser::T__1);
                break;
            }

            case 7: {
                enterOuterAlt(_localctx, 7);
                setState(425);
                match(xCEPParser::NOT_OP);
                setState(426);
                match(xCEPParser::EQUAL);
                break;
            }

            case 8: {
                enterOuterAlt(_localctx, 8);
                setState(427);
                match(xCEPParser::T__2);
                setState(428);
                match(xCEPParser::EQUAL);
                setState(429);
                match(xCEPParser::T__1);
                break;
            }

            case 9: {
                enterOuterAlt(_localctx, 9);
                setState(430);
                match(xCEPParser::EQUAL);
                break;
            }

            default: break;
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- LogicalOperatorContext ------------------------------------------------------------------

xCEPParser::LogicalOperatorContext::LogicalOperatorContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::LogicalOperatorContext::LOGAND() { return getToken(xCEPParser::LOGAND, 0); }

tree::TerminalNode* xCEPParser::LogicalOperatorContext::LOGXOR() { return getToken(xCEPParser::LOGXOR, 0); }

tree::TerminalNode* xCEPParser::LogicalOperatorContext::LOGOR() { return getToken(xCEPParser::LOGOR, 0); }

size_t xCEPParser::LogicalOperatorContext::getRuleIndex() const { return xCEPParser::RuleLogicalOperator; }

void xCEPParser::LogicalOperatorContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterLogicalOperator(this);
}

void xCEPParser::LogicalOperatorContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitLogicalOperator(this);
}

xCEPParser::LogicalOperatorContext* xCEPParser::logicalOperator() {
    LogicalOperatorContext* _localctx = _tracker.createInstance<LogicalOperatorContext>(_ctx, getState());
    enterRule(_localctx, 82, xCEPParser::RuleLogicalOperator);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(433);
        _la = _input->LA(1);
        if (!(((((_la - 70) & ~0x3fULL) == 0)
               && ((1ULL << (_la - 70))
                   & ((1ULL << (xCEPParser::LOGOR - 70)) | (1ULL << (xCEPParser::LOGAND - 70))
                      | (1ULL << (xCEPParser::LOGXOR - 70))))
                   != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- BitOperatorContext ------------------------------------------------------------------

xCEPParser::BitOperatorContext::BitOperatorContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::BitOperatorContext::LOGXOR() { return getToken(xCEPParser::LOGXOR, 0); }

size_t xCEPParser::BitOperatorContext::getRuleIndex() const { return xCEPParser::RuleBitOperator; }

void xCEPParser::BitOperatorContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterBitOperator(this);
}

void xCEPParser::BitOperatorContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitBitOperator(this);
}

xCEPParser::BitOperatorContext* xCEPParser::bitOperator() {
    BitOperatorContext* _localctx = _tracker.createInstance<BitOperatorContext>(_ctx, getState());
    enterRule(_localctx, 84, xCEPParser::RuleBitOperator);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(442);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case xCEPParser::T__2: {
                enterOuterAlt(_localctx, 1);
                setState(435);
                match(xCEPParser::T__2);
                setState(436);
                match(xCEPParser::T__2);
                break;
            }

            case xCEPParser::T__1: {
                enterOuterAlt(_localctx, 2);
                setState(437);
                match(xCEPParser::T__1);
                setState(438);
                match(xCEPParser::T__1);
                break;
            }

            case xCEPParser::T__3: {
                enterOuterAlt(_localctx, 3);
                setState(439);
                match(xCEPParser::T__3);
                break;
            }

            case xCEPParser::LOGXOR: {
                enterOuterAlt(_localctx, 4);
                setState(440);
                match(xCEPParser::LOGXOR);
                break;
            }

            case xCEPParser::T__4: {
                enterOuterAlt(_localctx, 5);
                setState(441);
                match(xCEPParser::T__4);
                break;
            }

            default: throw NoViableAltException(this);
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- MathOperatorContext ------------------------------------------------------------------

xCEPParser::MathOperatorContext::MathOperatorContext(ParserRuleContext* parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode* xCEPParser::MathOperatorContext::STAR() { return getToken(xCEPParser::STAR, 0); }

tree::TerminalNode* xCEPParser::MathOperatorContext::PLUS() { return getToken(xCEPParser::PLUS, 0); }

size_t xCEPParser::MathOperatorContext::getRuleIndex() const { return xCEPParser::RuleMathOperator; }

void xCEPParser::MathOperatorContext::enterRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->enterMathOperator(this);
}

void xCEPParser::MathOperatorContext::exitRule(tree::ParseTreeListener* listener) {
    auto parserListener = dynamic_cast<xCEPListener*>(listener);
    if (parserListener != nullptr)
        parserListener->exitMathOperator(this);
}

xCEPParser::MathOperatorContext* xCEPParser::mathOperator() {
    MathOperatorContext* _localctx = _tracker.createInstance<MathOperatorContext>(_ctx, getState());
    enterRule(_localctx, 86, xCEPParser::RuleMathOperator);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(444);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0)
               && ((1ULL << _la)
                   & ((1ULL << xCEPParser::T__0) | (1ULL << xCEPParser::T__5) | (1ULL << xCEPParser::T__6)
                      | (1ULL << xCEPParser::T__7) | (1ULL << xCEPParser::STAR) | (1ULL << xCEPParser::PLUS)))
                   != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    } catch (RecognitionException& e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

bool xCEPParser::sempred(RuleContext* context, size_t ruleIndex, size_t predicateIndex) {
    switch (ruleIndex) {
        case 28: return expressionSempred(dynamic_cast<ExpressionContext*>(context), predicateIndex);
        case 29: return predicateSempred(dynamic_cast<PredicateContext*>(context), predicateIndex);
        case 30: return expressionAtomSempred(dynamic_cast<ExpressionAtomContext*>(context), predicateIndex);

        default: break;
    }
    return true;
}

bool xCEPParser::expressionSempred(ExpressionContext* _localctx, size_t predicateIndex) {
    x_INFO("xCEPParser : expressionSempred {}", _localctx->getText());
    switch (predicateIndex) {
        case 0: return precpred(_ctx, 3);

        default: break;
    }
    return true;
}

bool xCEPParser::predicateSempred(PredicateContext* _localctx, size_t predicateIndex) {
    x_INFO("xCEPParser : predicateSempred {}", _localctx->getText());
    switch (predicateIndex) {
        case 1: return precpred(_ctx, 2);
        case 2: return precpred(_ctx, 4);
        case 3: return precpred(_ctx, 3);

        default: break;
    }
    return true;
}

bool xCEPParser::expressionAtomSempred(ExpressionAtomContext* _localctx, size_t predicateIndex) {
    x_INFO("xCEPParser : expressionAtomSempred {}", _localctx->getText());
    switch (predicateIndex) {
        case 4: return precpred(_ctx, 3);
        case 5: return precpred(_ctx, 2);

        default: break;
    }
    return true;
}

// Static vars and initialization.
std::vector<dfa::DFA> xCEPParser::_decisionToDFA;
atn::PredictionContextCache xCEPParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN xCEPParser::_atn;
std::vector<uint16_t> xCEPParser::_serializedATN;

std::vector<std::string> xCEPParser::_ruleNames = {"query",
                                                     "cepPattern",
                                                     "inputStreams",
                                                     "inputStream",
                                                     "compositeEventExpressions",
                                                     "whereExp",
                                                     "timeConstraints",
                                                     "interval",
                                                     "intervalType",
                                                     "option",
                                                     "outputExpression",
                                                     "outAttribute",
                                                     "sinkList",
                                                     "sink",
                                                     "listEvents",
                                                     "eventElem",
                                                     "event",
                                                     "quantifiers",
                                                     "iterMax",
                                                     "iterMin",
                                                     "consecutiveOption",
                                                     "operatorRule",
                                                     "sequence",
                                                     "contiguity",
                                                     "sinkType",
                                                     "nullNotnull",
                                                     "constant",
                                                     "expressions",
                                                     "expression",
                                                     "predicate",
                                                     "expressionAtom",
                                                     "eventAttribute",
                                                     "eventIteration",
                                                     "mathExpression",
                                                     "aggregation",
                                                     "attribute",
                                                     "attVal",
                                                     "boolRule",
                                                     "condition",
                                                     "unaryOperator",
                                                     "comparisonOperator",
                                                     "logicalOperator",
                                                     "bitOperator",
                                                     "mathOperator"};

std::vector<std::string> xCEPParser::_literalNames = {
    "",        "'-'",    "'>'",       "'<'",       "'&'",          "'|'",           "'/'",       "'%'",       "'--'",
    "",        "'FROM'", "'PATTERN'", "'WHERE'",   "'WITHIN'",     "'CONSUMING'",   "'RETURN'",  "'INTO'",    "'ALL'",
    "'ANY'",   "':='",   "','",       "'('",       "')'",          "'NOT'",         "'!'",       "'SEQ'",     "'NEXT'",
    "'AND'",   "'OR'",   "'*'",       "'+'",       "':'",          "'['",           "']'",       "'XOR'",     "'IN'",
    "'IS'",    "'NULL'", "'BETWEEN'", "'BINARY'",  "'TRUE'",       "'FALSE'",       "'UNKNOWN'", "'QUARTER'", "'MONTH'",
    "'DAY'",   "'HOUR'", "'MINUTE'",  "'WEEK'",    "'SECOND'",     "'MICROSECOND'", "'AS'",      "'='",       "'::'",
    "'Kafka'", "'File'", "'MQTT'",    "'Network'", "'NullOutput'", "'OPC'",         "'Print'",   "'ZMQ'",     "'.'",
    "'\"'",    "'AVG'",  "'SUM'",     "'MIN'",     "'MAX'",        "'COUNT'",       "'IF'",      "'||'",      "'&&'",
    "'^'",     "'NONE'"};

std::vector<std::string> xCEPParser::_symbolicNames = {"",
                                                         "",
                                                         "",
                                                         "",
                                                         "",
                                                         "",
                                                         "",
                                                         "",
                                                         "",
                                                         "WS",
                                                         "FROM",
                                                         "PATTERN",
                                                         "WHERE",
                                                         "WITHIN",
                                                         "CONSUMING",
                                                         "RETURN",
                                                         "INTO",
                                                         "ALL",
                                                         "ANY",
                                                         "SEP",
                                                         "COMMA",
                                                         "LPARENTHESIS",
                                                         "RPARENTHESIS",
                                                         "NOT",
                                                         "NOT_OP",
                                                         "SEQ",
                                                         "NEXT",
                                                         "AND",
                                                         "OR",
                                                         "STAR",
                                                         "PLUS",
                                                         "D_POINTS",
                                                         "LBRACKET",
                                                         "RBRACKET",
                                                         "XOR",
                                                         "IN",
                                                         "IS",
                                                         "NULLTOKEN",
                                                         "BETWEEN",
                                                         "BINARY",
                                                         "TRUE",
                                                         "FALSE",
                                                         "UNKNOWN",
                                                         "QUARTER",
                                                         "MONTH",
                                                         "DAY",
                                                         "HOUR",
                                                         "MINUTE",
                                                         "WEEK",
                                                         "SECOND",
                                                         "MICROSECOND",
                                                         "AS",
                                                         "EQUAL",
                                                         "SINKSEP",
                                                         "KAFKA",
                                                         "FILE",
                                                         "MQTT",
                                                         "NETWORK",
                                                         "NULLOUTPUT",
                                                         "OPC",
                                                         "PRINT",
                                                         "ZMQ",
                                                         "POINT",
                                                         "QUOTE",
                                                         "AVG",
                                                         "SUM",
                                                         "MIN",
                                                         "MAX",
                                                         "COUNT",
                                                         "IF",
                                                         "LOGOR",
                                                         "LOGAND",
                                                         "LOGXOR",
                                                         "NONE",
                                                         "INT",
                                                         "NAME",
                                                         "ID"};

dfa::Vocabulary xCEPParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> xCEPParser::_tokenNames;

xCEPParser::Initializer::Initializer() {
    for (size_t i = 0; i < _symbolicNames.size(); ++i) {
        std::string name = _vocabulary.getLiteralName(i);
        if (name.empty()) {
            name = _vocabulary.getSymbolicName(i);
        }

        if (name.empty()) {
            _tokenNames.push_back("<INVALID>");
        } else {
            _tokenNames.push_back(name);
        }
    }

    static const uint16_t serializedATNSegment0[] = {
        0x3,   0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 0x3,   0x4e,  0x1c1, 0x4,   0x2,   0x9,   0x2,
        0x4,   0x3,    0x9,    0x3,    0x4,    0x4,    0x9,    0x4,    0x4,    0x5,   0x9,   0x5,   0x4,   0x6,   0x9,   0x6,
        0x4,   0x7,    0x9,    0x7,    0x4,    0x8,    0x9,    0x8,    0x4,    0x9,   0x9,   0x9,   0x4,   0xa,   0x9,   0xa,
        0x4,   0xb,    0x9,    0xb,    0x4,    0xc,    0x9,    0xc,    0x4,    0xd,   0x9,   0xd,   0x4,   0xe,   0x9,   0xe,
        0x4,   0xf,    0x9,    0xf,    0x4,    0x10,   0x9,    0x10,   0x4,    0x11,  0x9,   0x11,  0x4,   0x12,  0x9,   0x12,
        0x4,   0x13,   0x9,    0x13,   0x4,    0x14,   0x9,    0x14,   0x4,    0x15,  0x9,   0x15,  0x4,   0x16,  0x9,   0x16,
        0x4,   0x17,   0x9,    0x17,   0x4,    0x18,   0x9,    0x18,   0x4,    0x19,  0x9,   0x19,  0x4,   0x1a,  0x9,   0x1a,
        0x4,   0x1b,   0x9,    0x1b,   0x4,    0x1c,   0x9,    0x1c,   0x4,    0x1d,  0x9,   0x1d,  0x4,   0x1e,  0x9,   0x1e,
        0x4,   0x1f,   0x9,    0x1f,   0x4,    0x20,   0x9,    0x20,   0x4,    0x21,  0x9,   0x21,  0x4,   0x22,  0x9,   0x22,
        0x4,   0x23,   0x9,    0x23,   0x4,    0x24,   0x9,    0x24,   0x4,    0x25,  0x9,   0x25,  0x4,   0x26,  0x9,   0x26,
        0x4,   0x27,   0x9,    0x27,   0x4,    0x28,   0x9,    0x28,   0x4,    0x29,  0x9,   0x29,  0x4,   0x2a,  0x9,   0x2a,
        0x4,   0x2b,   0x9,    0x2b,   0x4,    0x2c,   0x9,    0x2c,   0x4,    0x2d,  0x9,   0x2d,  0x3,   0x2,   0x6,   0x2,
        0x5c,  0xa,    0x2,    0xd,    0x2,    0xe,    0x2,    0x5d,   0x3,    0x2,   0x3,   0x2,   0x3,   0x3,   0x3,   0x3,
        0x3,   0x3,    0x3,    0x3,    0x3,    0x3,    0x3,    0x3,    0x3,    0x3,   0x3,   0x3,   0x5,   0x3,   0x6a,  0xa,
        0x3,   0x3,    0x3,    0x3,    0x3,    0x5,    0x3,    0x6e,   0xa,    0x3,   0x3,   0x3,   0x3,   0x3,   0x5,   0x3,
        0x72,  0xa,    0x3,    0x3,    0x3,    0x3,    0x3,    0x5,    0x3,    0x76,  0xa,   0x3,   0x3,   0x3,   0x3,   0x3,
        0x3,   0x3,    0x3,    0x4,    0x3,    0x4,    0x3,    0x4,    0x7,    0x4,   0x7e,  0xa,   0x4,   0xc,   0x4,   0xe,
        0x4,   0x81,   0xb,    0x4,    0x3,    0x5,    0x3,    0x5,    0x3,    0x5,   0x5,   0x5,   0x86,  0xa,   0x5,   0x3,
        0x6,   0x3,    0x6,    0x3,    0x6,    0x3,    0x6,    0x3,    0x7,    0x3,   0x7,   0x3,   0x8,   0x3,   0x8,   0x3,
        0x8,   0x3,    0x8,    0x3,    0x9,    0x3,    0x9,    0x3,    0x9,    0x3,   0xa,   0x3,   0xa,   0x3,   0xb,   0x3,
        0xb,   0x3,    0xc,    0x3,    0xc,    0x3,    0xc,    0x3,    0xc,    0x3,   0xc,   0x3,   0xc,   0x7,   0xc,   0x9f,
        0xa,   0xc,    0xc,    0xc,    0xe,    0xc,    0xa2,   0xb,    0xc,    0x3,   0xc,   0x3,   0xc,   0x3,   0xd,   0x3,
        0xd,   0x3,    0xd,    0x3,    0xd,    0x3,    0xe,    0x3,    0xe,    0x3,   0xe,   0x7,   0xe,   0xad,  0xa,   0xe,
        0xc,   0xe,    0xe,    0xe,    0xb0,   0xb,    0xe,    0x3,    0xf,    0x3,   0xf,   0x3,   0xf,   0x3,   0xf,   0x3,
        0x10,  0x3,    0x10,   0x3,    0x10,   0x3,    0x10,   0x7,    0x10,   0xba,  0xa,   0x10,  0xc,   0x10,  0xe,   0x10,
        0xbd,  0xb,    0x10,   0x3,    0x11,   0x5,    0x11,   0xc0,   0xa,    0x11,  0x3,   0x11,  0x3,   0x11,  0x5,   0x11,
        0xc4,  0xa,    0x11,   0x3,    0x11,   0x3,    0x11,   0x3,    0x11,   0x3,   0x11,  0x5,   0x11,  0xca,  0xa,   0x11,
        0x3,   0x12,   0x3,    0x12,   0x5,    0x12,   0xce,   0xa,    0x12,   0x3,   0x13,  0x3,   0x13,  0x3,   0x13,  0x3,
        0x13,  0x5,    0x13,   0xd4,   0xa,    0x13,   0x3,    0x13,   0x3,    0x13,  0x5,   0x13,  0xd8,  0xa,   0x13,  0x3,
        0x13,  0x3,    0x13,   0x3,    0x13,   0x5,    0x13,   0xdd,   0xa,    0x13,  0x3,   0x13,  0x3,   0x13,  0x3,   0x13,
        0x3,   0x13,   0x3,    0x13,   0x5,    0x13,   0xe4,   0xa,    0x13,   0x3,   0x14,  0x3,   0x14,  0x3,   0x15,  0x3,
        0x15,  0x3,    0x16,   0x5,    0x16,   0xeb,   0xa,    0x16,   0x3,    0x16,  0x3,   0x16,  0x3,   0x17,  0x3,   0x17,
        0x3,   0x17,   0x5,    0x17,   0xf2,   0xa,    0x17,   0x3,    0x18,   0x3,   0x18,  0x5,   0x18,  0xf6,  0xa,   0x18,
        0x3,   0x19,   0x3,    0x19,   0x3,    0x19,   0x5,    0x19,   0xfb,   0xa,   0x19,  0x3,   0x1a,  0x3,   0x1a,  0x3,
        0x1b,  0x5,    0x1b,   0x100,  0xa,    0x1b,   0x3,    0x1b,   0x3,    0x1b,  0x3,   0x1c,  0x3,   0x1c,  0x3,   0x1c,
        0x3,   0x1c,   0x3,    0x1c,   0x5,    0x1c,   0x109,  0xa,    0x1c,   0x3,   0x1d,  0x3,   0x1d,  0x3,   0x1d,  0x7,
        0x1d,  0x10e,  0xa,    0x1d,   0xc,    0x1d,   0xe,    0x1d,   0x111,  0xb,   0x1d,  0x3,   0x1e,  0x3,   0x1e,  0x3,
        0x1e,  0x3,    0x1e,   0x3,    0x1e,   0x3,    0x1e,   0x5,    0x1e,   0x119, 0xa,   0x1e,  0x3,   0x1e,  0x3,   0x1e,
        0x3,   0x1e,   0x5,    0x1e,   0x11e,  0xa,    0x1e,   0x3,    0x1e,   0x3,   0x1e,  0x3,   0x1e,  0x3,   0x1e,  0x7,
        0x1e,  0x124,  0xa,    0x1e,   0xc,    0x1e,   0xe,    0x1e,   0x127,  0xb,   0x1e,  0x3,   0x1f,  0x3,   0x1f,  0x3,
        0x1f,  0x3,    0x1f,   0x3,    0x1f,   0x3,    0x1f,   0x3,    0x1f,   0x3,   0x1f,  0x3,   0x1f,  0x5,   0x1f,  0x132,
        0xa,   0x1f,   0x3,    0x1f,   0x3,    0x1f,   0x3,    0x1f,   0x3,    0x1f,  0x3,   0x1f,  0x3,   0x1f,  0x3,   0x1f,
        0x3,   0x1f,   0x7,    0x1f,   0x13c,  0xa,    0x1f,   0xc,    0x1f,   0xe,   0x1f,  0x13f, 0xb,   0x1f,  0x3,   0x20,
        0x3,   0x20,   0x3,    0x20,   0x3,    0x20,   0x3,    0x20,   0x3,    0x20,  0x3,   0x20,  0x3,   0x20,  0x3,   0x20,
        0x3,   0x20,   0x3,    0x20,   0x7,    0x20,   0x14c,  0xa,    0x20,   0xc,   0x20,  0xe,   0x20,  0x14f, 0xb,   0x20,
        0x3,   0x20,   0x3,    0x20,   0x3,    0x20,   0x5,    0x20,   0x154,  0xa,   0x20,  0x3,   0x20,  0x3,   0x20,  0x3,
        0x20,  0x3,    0x20,   0x3,    0x20,   0x3,    0x20,   0x3,    0x20,   0x3,   0x20,  0x7,   0x20,  0x15e, 0xa,   0x20,
        0xc,   0x20,   0xe,    0x20,   0x161,  0xb,    0x20,   0x3,    0x21,   0x3,   0x21,  0x3,   0x21,  0x3,   0x21,  0x3,
        0x21,  0x3,    0x21,   0x3,    0x21,   0x3,    0x21,   0x5,    0x21,   0x16b, 0xa,   0x21,  0x3,   0x21,  0x3,   0x21,
        0x3,   0x21,   0x5,    0x21,   0x170,  0xa,    0x21,   0x3,    0x22,   0x3,   0x22,  0x3,   0x22,  0x5,   0x22,  0x175,
        0xa,   0x22,   0x3,    0x22,   0x3,    0x22,   0x3,    0x23,   0x3,    0x23,  0x3,   0x23,  0x3,   0x23,  0x3,   0x23,
        0x3,   0x23,   0x3,    0x23,   0x5,    0x23,   0x180,  0xa,    0x23,   0x5,   0x23,  0x182, 0xa,   0x23,  0x3,   0x24,
        0x3,   0x24,   0x3,    0x24,   0x3,    0x24,   0x3,    0x24,   0x3,    0x24,  0x5,   0x24,  0x18a, 0xa,   0x24,  0x3,
        0x25,  0x3,    0x25,   0x3,    0x26,   0x3,    0x26,   0x3,    0x26,   0x3,   0x26,  0x3,   0x26,  0x3,   0x26,  0x5,
        0x26,  0x194,  0xa,    0x26,   0x3,    0x27,   0x3,    0x27,   0x3,    0x28,  0x3,   0x28,  0x3,   0x28,  0x3,   0x28,
        0x3,   0x28,   0x3,    0x28,   0x3,    0x28,   0x3,    0x28,   0x3,    0x29,  0x3,   0x29,  0x3,   0x2a,  0x3,   0x2a,
        0x3,   0x2a,   0x3,    0x2a,   0x3,    0x2a,   0x3,    0x2a,   0x3,    0x2a,  0x3,   0x2a,  0x3,   0x2a,  0x3,   0x2a,
        0x3,   0x2a,   0x3,    0x2a,   0x3,    0x2a,   0x3,    0x2a,   0x3,    0x2a,  0x3,   0x2a,  0x5,   0x2a,  0x1b2, 0xa,
        0x2a,  0x3,    0x2b,   0x3,    0x2b,   0x3,    0x2c,   0x3,    0x2c,   0x3,   0x2c,  0x3,   0x2c,  0x3,   0x2c,  0x3,
        0x2c,  0x3,    0x2c,   0x5,    0x2c,   0x1bd,  0xa,    0x2c,   0x3,    0x2d,  0x3,   0x2d,  0x3,   0x2d,  0x2,   0x5,
        0x3a,  0x3c,   0x3e,   0x2e,   0x2,    0x4,    0x6,    0x8,    0xa,    0xc,   0xe,   0x10,  0x12,  0x14,  0x16,  0x18,
        0x1a,  0x1c,   0x1e,   0x20,   0x22,   0x24,   0x26,   0x28,   0x2a,   0x2c,  0x2e,  0x30,  0x32,  0x34,  0x36,  0x38,
        0x3a,  0x3c,   0x3e,   0x40,   0x42,   0x44,   0x46,   0x48,   0x4a,   0x4c,  0x4e,  0x50,  0x52,  0x54,  0x56,  0x58,
        0x2,   0xa,    0x3,    0x2,    0x2d,   0x34,   0x4,    0x2,    0x13,   0x13,  0x4b,  0x4b,  0x3,   0x2,   0x38,  0x3f,
        0x3,   0x2,    0x2a,   0x2c,   0x3,    0x2,    0x2a,   0x2b,   0x5,    0x2,   0x3,   0x3,   0x19,  0x19,  0x20,  0x20,
        0x3,   0x2,    0x48,   0x4a,   0x5,    0x2,    0x3,    0x3,    0x8,    0xa,   0x1f,  0x20,  0x2,   0x1db, 0x2,   0x5b,
        0x3,   0x2,    0x2,    0x2,    0x4,    0x61,   0x3,    0x2,    0x2,    0x2,   0x6,   0x7a,  0x3,   0x2,   0x2,   0x2,
        0x8,   0x82,   0x3,    0x2,    0x2,    0x2,    0xa,    0x87,   0x3,    0x2,   0x2,   0x2,   0xc,   0x8b,  0x3,   0x2,
        0x2,   0x2,    0xe,    0x8d,   0x3,    0x2,    0x2,    0x2,    0x10,   0x91,  0x3,   0x2,   0x2,   0x2,   0x12,  0x94,
        0x3,   0x2,    0x2,    0x2,    0x14,   0x96,   0x3,    0x2,    0x2,    0x2,   0x16,  0x98,  0x3,   0x2,   0x2,   0x2,
        0x18,  0xa5,   0x3,    0x2,    0x2,    0x2,    0x1a,   0xa9,   0x3,    0x2,   0x2,   0x2,   0x1c,  0xb1,  0x3,   0x2,
        0x2,   0x2,    0x1e,   0xb5,   0x3,    0x2,    0x2,    0x2,    0x20,   0xc9,  0x3,   0x2,   0x2,   0x2,   0x22,  0xcb,
        0x3,   0x2,    0x2,    0x2,    0x24,   0xe3,   0x3,    0x2,    0x2,    0x2,   0x26,  0xe5,  0x3,   0x2,   0x2,   0x2,
        0x28,  0xe7,   0x3,    0x2,    0x2,    0x2,    0x2a,   0xea,   0x3,    0x2,   0x2,   0x2,   0x2c,  0xf1,  0x3,   0x2,
        0x2,   0x2,    0x2e,   0xf5,   0x3,    0x2,    0x2,    0x2,    0x30,   0xfa,  0x3,   0x2,   0x2,   0x2,   0x32,  0xfc,
        0x3,   0x2,    0x2,    0x2,    0x34,   0xff,   0x3,    0x2,    0x2,    0x2,   0x36,  0x108, 0x3,   0x2,   0x2,   0x2,
        0x38,  0x10a,  0x3,    0x2,    0x2,    0x2,    0x3a,   0x11d,  0x3,    0x2,   0x2,   0x2,   0x3c,  0x128, 0x3,   0x2,
        0x2,   0x2,    0x3e,   0x153,  0x3,    0x2,    0x2,    0x2,    0x40,   0x16f, 0x3,   0x2,   0x2,   0x2,   0x42,  0x171,
        0x3,   0x2,    0x2,    0x2,    0x44,   0x181,  0x3,    0x2,    0x2,    0x2,   0x46,  0x189, 0x3,   0x2,   0x2,   0x2,
        0x48,  0x18b,  0x3,    0x2,    0x2,    0x2,    0x4a,   0x193,  0x3,    0x2,   0x2,   0x2,   0x4c,  0x195, 0x3,   0x2,
        0x2,   0x2,    0x4e,   0x197,  0x3,    0x2,    0x2,    0x2,    0x50,   0x19f, 0x3,   0x2,   0x2,   0x2,   0x52,  0x1b1,
        0x3,   0x2,    0x2,    0x2,    0x54,   0x1b3,  0x3,    0x2,    0x2,    0x2,   0x56,  0x1bc, 0x3,   0x2,   0x2,   0x2,
        0x58,  0x1be,  0x3,    0x2,    0x2,    0x2,    0x5a,   0x5c,   0x5,    0x4,   0x3,   0x2,   0x5b,  0x5a,  0x3,   0x2,
        0x2,   0x2,    0x5c,   0x5d,   0x3,    0x2,    0x2,    0x2,    0x5d,   0x5b,  0x3,   0x2,   0x2,   0x2,   0x5d,  0x5e,
        0x3,   0x2,    0x2,    0x2,    0x5e,   0x5f,   0x3,    0x2,    0x2,    0x2,   0x5f,  0x60,  0x7,   0x2,   0x2,   0x3,
        0x60,  0x3,    0x3,    0x2,    0x2,    0x2,    0x61,   0x62,   0x7,    0xd,   0x2,   0x2,   0x62,  0x63,  0x7,   0x4d,
        0x2,   0x2,    0x63,   0x64,   0x7,    0x15,   0x2,    0x2,    0x64,   0x65,  0x5,   0xa,   0x6,   0x2,   0x65,  0x66,
        0x7,   0xc,    0x2,    0x2,    0x66,   0x69,   0x5,    0x6,    0x4,    0x2,   0x67,  0x68,  0x7,   0xe,   0x2,   0x2,
        0x68,  0x6a,   0x5,    0xc,    0x7,    0x2,    0x69,   0x67,   0x3,    0x2,   0x2,   0x2,   0x69,  0x6a,  0x3,   0x2,
        0x2,   0x2,    0x6a,   0x6d,   0x3,    0x2,    0x2,    0x2,    0x6b,   0x6c,  0x7,   0xf,   0x2,   0x2,   0x6c,  0x6e,
        0x5,   0xe,    0x8,    0x2,    0x6d,   0x6b,   0x3,    0x2,    0x2,    0x2,   0x6d,  0x6e,  0x3,   0x2,   0x2,   0x2,
        0x6e,  0x71,   0x3,    0x2,    0x2,    0x2,    0x6f,   0x70,   0x7,    0x10,  0x2,   0x2,   0x70,  0x72,  0x5,   0x14,
        0xb,   0x2,    0x71,   0x6f,   0x3,    0x2,    0x2,    0x2,    0x71,   0x72,  0x3,   0x2,   0x2,   0x2,   0x72,  0x75,
        0x3,   0x2,    0x2,    0x2,    0x73,   0x74,   0x7,    0x11,   0x2,    0x2,   0x74,  0x76,  0x5,   0x16,  0xc,   0x2,
        0x75,  0x73,   0x3,    0x2,    0x2,    0x2,    0x75,   0x76,   0x3,    0x2,   0x2,   0x2,   0x76,  0x77,  0x3,   0x2,
        0x2,   0x2,    0x77,   0x78,   0x7,    0x12,   0x2,    0x2,    0x78,   0x79,  0x5,   0x1a,  0xe,   0x2,   0x79,  0x5,
        0x3,   0x2,    0x2,    0x2,    0x7a,   0x7f,   0x5,    0x8,    0x5,    0x2,   0x7b,  0x7c,  0x7,   0x16,  0x2,   0x2,
        0x7c,  0x7e,   0x5,    0x8,    0x5,    0x2,    0x7d,   0x7b,   0x3,    0x2,   0x2,   0x2,   0x7e,  0x81,  0x3,   0x2,
        0x2,   0x2,    0x7f,   0x7d,   0x3,    0x2,    0x2,    0x2,    0x7f,   0x80,  0x3,   0x2,   0x2,   0x2,   0x80,  0x7,
        0x3,   0x2,    0x2,    0x2,    0x81,   0x7f,   0x3,    0x2,    0x2,    0x2,   0x82,  0x85,  0x7,   0x4d,  0x2,   0x2,
        0x83,  0x84,   0x7,    0x35,   0x2,    0x2,    0x84,   0x86,   0x7,    0x4d,  0x2,   0x2,   0x85,  0x83,  0x3,   0x2,
        0x2,   0x2,    0x85,   0x86,   0x3,    0x2,    0x2,    0x2,    0x86,   0x9,   0x3,   0x2,   0x2,   0x2,   0x87,  0x88,
        0x7,   0x17,   0x2,    0x2,    0x88,   0x89,   0x5,    0x1e,   0x10,   0x2,   0x89,  0x8a,  0x7,   0x18,  0x2,   0x2,
        0x8a,  0xb,    0x3,    0x2,    0x2,    0x2,    0x8b,   0x8c,   0x5,    0x3a,  0x1e,  0x2,   0x8c,  0xd,   0x3,   0x2,
        0x2,   0x2,    0x8d,   0x8e,   0x7,    0x22,   0x2,    0x2,    0x8e,   0x8f,  0x5,   0x10,  0x9,   0x2,   0x8f,  0x90,
        0x7,   0x23,   0x2,    0x2,    0x90,   0xf,    0x3,    0x2,    0x2,    0x2,   0x91,  0x92,  0x7,   0x4c,  0x2,   0x2,
        0x92,  0x93,   0x5,    0x12,   0xa,    0x2,    0x93,   0x11,   0x3,    0x2,   0x2,   0x2,   0x94,  0x95,  0x9,   0x2,
        0x2,   0x2,    0x95,   0x13,   0x3,    0x2,    0x2,    0x2,    0x96,   0x97,  0x9,   0x3,   0x2,   0x2,   0x97,  0x15,
        0x3,   0x2,    0x2,    0x2,    0x98,   0x99,   0x7,    0x4d,   0x2,    0x2,   0x99,  0x9a,  0x7,   0x15,  0x2,   0x2,
        0x9a,  0x9b,   0x7,    0x22,   0x2,    0x2,    0x9b,   0xa0,   0x5,    0x18,  0xd,   0x2,   0x9c,  0x9d,  0x7,   0x16,
        0x2,   0x2,    0x9d,   0x9f,   0x5,    0x18,   0xd,    0x2,    0x9e,   0x9c,  0x3,   0x2,   0x2,   0x2,   0x9f,  0xa2,
        0x3,   0x2,    0x2,    0x2,    0xa0,   0x9e,   0x3,    0x2,    0x2,    0x2,   0xa0,  0xa1,  0x3,   0x2,   0x2,   0x2,
        0xa1,  0xa3,   0x3,    0x2,    0x2,    0x2,    0xa2,   0xa0,   0x3,    0x2,   0x2,   0x2,   0xa3,  0xa4,  0x7,   0x23,
        0x2,   0x2,    0xa4,   0x17,   0x3,    0x2,    0x2,    0x2,    0xa5,   0xa6,  0x7,   0x4d,  0x2,   0x2,   0xa6,  0xa7,
        0x7,   0x36,   0x2,    0x2,    0xa7,   0xa8,   0x5,    0x4a,   0x26,   0x2,   0xa8,  0x19,  0x3,   0x2,   0x2,   0x2,
        0xa9,  0xae,   0x5,    0x1c,   0xf,    0x2,    0xaa,   0xab,   0x7,    0x16,  0x2,   0x2,   0xab,  0xad,  0x5,   0x1c,
        0xf,   0x2,    0xac,   0xaa,   0x3,    0x2,    0x2,    0x2,    0xad,   0xb0,  0x3,   0x2,   0x2,   0x2,   0xae,  0xac,
        0x3,   0x2,    0x2,    0x2,    0xae,   0xaf,   0x3,    0x2,    0x2,    0x2,   0xaf,  0x1b,  0x3,   0x2,   0x2,   0x2,
        0xb0,  0xae,   0x3,    0x2,    0x2,    0x2,    0xb1,   0xb2,   0x5,    0x32,  0x1a,  0x2,   0xb2,  0xb3,  0x7,   0x37,
        0x2,   0x2,    0xb3,   0xb4,   0x7,    0x4d,   0x2,    0x2,    0xb4,   0x1d,  0x3,   0x2,   0x2,   0x2,   0xb5,  0xbb,
        0x5,   0x20,   0x11,   0x2,    0xb6,   0xb7,   0x5,    0x2c,   0x17,   0x2,   0xb7,  0xb8,  0x5,   0x20,  0x11,  0x2,
        0xb8,  0xba,   0x3,    0x2,    0x2,    0x2,    0xb9,   0xb6,   0x3,    0x2,   0x2,   0x2,   0xba,  0xbd,  0x3,   0x2,
        0x2,   0x2,    0xbb,   0xb9,   0x3,    0x2,    0x2,    0x2,    0xbb,   0xbc,  0x3,   0x2,   0x2,   0x2,   0xbc,  0x1f,
        0x3,   0x2,    0x2,    0x2,    0xbd,   0xbb,   0x3,    0x2,    0x2,    0x2,   0xbe,  0xc0,  0x7,   0x19,  0x2,   0x2,
        0xbf,  0xbe,   0x3,    0x2,    0x2,    0x2,    0xbf,   0xc0,   0x3,    0x2,   0x2,   0x2,   0xc0,  0xc1,  0x3,   0x2,
        0x2,   0x2,    0xc1,   0xca,   0x5,    0x22,   0x12,   0x2,    0xc2,   0xc4,  0x7,   0x19,  0x2,   0x2,   0xc3,  0xc2,
        0x3,   0x2,    0x2,    0x2,    0xc3,   0xc4,   0x3,    0x2,    0x2,    0x2,   0xc4,  0xc5,  0x3,   0x2,   0x2,   0x2,
        0xc5,  0xc6,   0x7,    0x17,   0x2,    0x2,    0xc6,   0xc7,   0x5,    0x1e,  0x10,  0x2,   0xc7,  0xc8,  0x7,   0x18,
        0x2,   0x2,    0xc8,   0xca,   0x3,    0x2,    0x2,    0x2,    0xc9,   0xbf,  0x3,   0x2,   0x2,   0x2,   0xc9,  0xc3,
        0x3,   0x2,    0x2,    0x2,    0xca,   0x21,   0x3,    0x2,    0x2,    0x2,   0xcb,  0xcd,  0x7,   0x4d,  0x2,   0x2,
        0xcc,  0xce,   0x5,    0x24,   0x13,   0x2,    0xcd,   0xcc,   0x3,    0x2,   0x2,   0x2,   0xcd,  0xce,  0x3,   0x2,
        0x2,   0x2,    0xce,   0x23,   0x3,    0x2,    0x2,    0x2,    0xcf,   0xe4,  0x7,   0x1f,  0x2,   0x2,   0xd0,  0xe4,
        0x7,   0x20,   0x2,    0x2,    0xd1,   0xd3,   0x7,    0x22,   0x2,    0x2,   0xd2,  0xd4,  0x5,   0x2a,  0x16,  0x2,
        0xd3,  0xd2,   0x3,    0x2,    0x2,    0x2,    0xd3,   0xd4,   0x3,    0x2,   0x2,   0x2,   0xd4,  0xd5,  0x3,   0x2,
        0x2,   0x2,    0xd5,   0xd7,   0x7,    0x4c,   0x2,    0x2,    0xd6,   0xd8,  0x7,   0x20,  0x2,   0x2,   0xd7,  0xd6,
        0x3,   0x2,    0x2,    0x2,    0xd7,   0xd8,   0x3,    0x2,    0x2,    0x2,   0xd8,  0xd9,  0x3,   0x2,   0x2,   0x2,
        0xd9,  0xe4,   0x7,    0x23,   0x2,    0x2,    0xda,   0xdc,   0x7,    0x22,  0x2,   0x2,   0xdb,  0xdd,  0x5,   0x2a,
        0x16,  0x2,    0xdc,   0xdb,   0x3,    0x2,    0x2,    0x2,    0xdc,   0xdd,  0x3,   0x2,   0x2,   0x2,   0xdd,  0xde,
        0x3,   0x2,    0x2,    0x2,    0xde,   0xdf,   0x5,    0x28,   0x15,   0x2,   0xdf,  0xe0,  0x7,   0x21,  0x2,   0x2,
        0xe0,  0xe1,   0x5,    0x26,   0x14,   0x2,    0xe1,   0xe2,   0x7,    0x23,  0x2,   0x2,   0xe2,  0xe4,  0x3,   0x2,
        0x2,   0x2,    0xe3,   0xcf,   0x3,    0x2,    0x2,    0x2,    0xe3,   0xd0,  0x3,   0x2,   0x2,   0x2,   0xe3,  0xd1,
        0x3,   0x2,    0x2,    0x2,    0xe3,   0xda,   0x3,    0x2,    0x2,    0x2,   0xe4,  0x25,  0x3,   0x2,   0x2,   0x2,
        0xe5,  0xe6,   0x7,    0x4c,   0x2,    0x2,    0xe6,   0x27,   0x3,    0x2,   0x2,   0x2,   0xe7,  0xe8,  0x7,   0x4c,
        0x2,   0x2,    0xe8,   0x29,   0x3,    0x2,    0x2,    0x2,    0xe9,   0xeb,  0x7,   0x14,  0x2,   0x2,   0xea,  0xe9,
        0x3,   0x2,    0x2,    0x2,    0xea,   0xeb,   0x3,    0x2,    0x2,    0x2,   0xeb,  0xec,  0x3,   0x2,   0x2,   0x2,
        0xec,  0xed,   0x7,    0x1c,   0x2,    0x2,    0xed,   0x2b,   0x3,    0x2,   0x2,   0x2,   0xee,  0xf2,  0x7,   0x1d,
        0x2,   0x2,    0xef,   0xf2,   0x7,    0x1e,   0x2,    0x2,    0xf0,   0xf2,  0x5,   0x2e,  0x18,  0x2,   0xf1,  0xee,
        0x3,   0x2,    0x2,    0x2,    0xf1,   0xef,   0x3,    0x2,    0x2,    0x2,   0xf1,  0xf0,  0x3,   0x2,   0x2,   0x2,
        0xf2,  0x2d,   0x3,    0x2,    0x2,    0x2,    0xf3,   0xf6,   0x7,    0x1b,  0x2,   0x2,   0xf4,  0xf6,  0x5,   0x30,
        0x19,  0x2,    0xf5,   0xf3,   0x3,    0x2,    0x2,    0x2,    0xf5,   0xf4,  0x3,   0x2,   0x2,   0x2,   0xf6,  0x2f,
        0x3,   0x2,    0x2,    0x2,    0xf7,   0xfb,   0x7,    0x1c,   0x2,    0x2,   0xf8,  0xf9,  0x7,   0x14,  0x2,   0x2,
        0xf9,  0xfb,   0x7,    0x1c,   0x2,    0x2,    0xfa,   0xf7,   0x3,    0x2,   0x2,   0x2,   0xfa,  0xf8,  0x3,   0x2,
        0x2,   0x2,    0xfb,   0x31,   0x3,    0x2,    0x2,    0x2,    0xfc,   0xfd,  0x9,   0x4,   0x2,   0x2,   0xfd,  0x33,
        0x3,   0x2,    0x2,    0x2,    0xfe,   0x100,  0x7,    0x19,   0x2,    0x2,   0xff,  0xfe,  0x3,   0x2,   0x2,   0x2,
        0xff,  0x100,  0x3,    0x2,    0x2,    0x2,    0x100,  0x101,  0x3,    0x2,   0x2,   0x2,   0x101, 0x102, 0x7,   0x27,
        0x2,   0x2,    0x102,  0x35,   0x3,    0x2,    0x2,    0x2,    0x103,  0x104, 0x7,   0x41,  0x2,   0x2,   0x104, 0x105,
        0x7,   0x4d,   0x2,    0x2,    0x105,  0x109,  0x7,    0x41,   0x2,    0x2,   0x106, 0x109, 0x7,   0x4c,  0x2,   0x2,
        0x107, 0x109,  0x7,    0x4d,   0x2,    0x2,    0x108,  0x103,  0x3,    0x2,   0x2,   0x2,   0x108, 0x106, 0x3,   0x2,
        0x2,   0x2,    0x108,  0x107,  0x3,    0x2,    0x2,    0x2,    0x109,  0x37,  0x3,   0x2,   0x2,   0x2,   0x10a, 0x10f,
        0x5,   0x3a,   0x1e,   0x2,    0x10b,  0x10c,  0x7,    0x16,   0x2,    0x2,   0x10c, 0x10e, 0x5,   0x3a,  0x1e,  0x2,
        0x10d, 0x10b,  0x3,    0x2,    0x2,    0x2,    0x10e,  0x111,  0x3,    0x2,   0x2,   0x2,   0x10f, 0x10d, 0x3,   0x2,
        0x2,   0x2,    0x10f,  0x110,  0x3,    0x2,    0x2,    0x2,    0x110,  0x39,  0x3,   0x2,   0x2,   0x2,   0x111, 0x10f,
        0x3,   0x2,    0x2,    0x2,    0x112,  0x113,  0x8,    0x1e,   0x1,    0x2,   0x113, 0x114, 0x7,   0x1a,  0x2,   0x2,
        0x114, 0x11e,  0x5,    0x3a,   0x1e,   0x6,    0x115,  0x116,  0x5,    0x3c,  0x1f,  0x2,   0x116, 0x118, 0x7,   0x26,
        0x2,   0x2,    0x117,  0x119,  0x7,    0x19,   0x2,    0x2,    0x118,  0x117, 0x3,   0x2,   0x2,   0x2,   0x118, 0x119,
        0x3,   0x2,    0x2,    0x2,    0x119,  0x11a,  0x3,    0x2,    0x2,    0x2,   0x11a, 0x11b, 0x9,   0x5,   0x2,   0x2,
        0x11b, 0x11e,  0x3,    0x2,    0x2,    0x2,    0x11c,  0x11e,  0x5,    0x3c,  0x1f,  0x2,   0x11d, 0x112, 0x3,   0x2,
        0x2,   0x2,    0x11d,  0x115,  0x3,    0x2,    0x2,    0x2,    0x11d,  0x11c, 0x3,   0x2,   0x2,   0x2,   0x11e, 0x125,
        0x3,   0x2,    0x2,    0x2,    0x11f,  0x120,  0xc,    0x5,    0x2,    0x2,   0x120, 0x121, 0x5,   0x54,  0x2b,  0x2,
        0x121, 0x122,  0x5,    0x3a,   0x1e,   0x6,    0x122,  0x124,  0x3,    0x2,   0x2,   0x2,   0x123, 0x11f, 0x3,   0x2,
        0x2,   0x2,    0x124,  0x127,  0x3,    0x2,    0x2,    0x2,    0x125,  0x123, 0x3,   0x2,   0x2,   0x2,   0x125, 0x126,
        0x3,   0x2,    0x2,    0x2,    0x126,  0x3b,   0x3,    0x2,    0x2,    0x2,   0x127, 0x125, 0x3,   0x2,   0x2,   0x2,
        0x128, 0x129,  0x8,    0x1f,   0x1,    0x2,    0x129,  0x12a,  0x5,    0x3e,  0x20,  0x2,   0x12a, 0x13d, 0x3,   0x2,
        0x2,   0x2,    0x12b,  0x12c,  0xc,    0x4,    0x2,    0x2,    0x12c,  0x12d, 0x5,   0x52,  0x2a,  0x2,   0x12d, 0x12e,
        0x5,   0x3c,   0x1f,   0x5,    0x12e,  0x13c,  0x3,    0x2,    0x2,    0x2,   0x12f, 0x131, 0xc,   0x6,   0x2,   0x2,
        0x130, 0x132,  0x7,    0x19,   0x2,    0x2,    0x131,  0x130,  0x3,    0x2,   0x2,   0x2,   0x131, 0x132, 0x3,   0x2,
        0x2,   0x2,    0x132,  0x133,  0x3,    0x2,    0x2,    0x2,    0x133,  0x134, 0x7,   0x25,  0x2,   0x2,   0x134, 0x135,
        0x7,   0x17,   0x2,    0x2,    0x135,  0x136,  0x5,    0x38,   0x1d,   0x2,   0x136, 0x137, 0x7,   0x18,  0x2,   0x2,
        0x137, 0x13c,  0x3,    0x2,    0x2,    0x2,    0x138,  0x139,  0xc,    0x5,   0x2,   0x2,   0x139, 0x13a, 0x7,   0x26,
        0x2,   0x2,    0x13a,  0x13c,  0x5,    0x34,   0x1b,   0x2,    0x13b,  0x12b, 0x3,   0x2,   0x2,   0x2,   0x13b, 0x12f,
        0x3,   0x2,    0x2,    0x2,    0x13b,  0x138,  0x3,    0x2,    0x2,    0x2,   0x13c, 0x13f, 0x3,   0x2,   0x2,   0x2,
        0x13d, 0x13b,  0x3,    0x2,    0x2,    0x2,    0x13d,  0x13e,  0x3,    0x2,   0x2,   0x2,   0x13e, 0x3d,  0x3,   0x2,
        0x2,   0x2,    0x13f,  0x13d,  0x3,    0x2,    0x2,    0x2,    0x140,  0x141, 0x8,   0x20,  0x1,   0x2,   0x141, 0x154,
        0x5,   0x40,   0x21,   0x2,    0x142,  0x143,  0x5,    0x50,   0x29,   0x2,   0x143, 0x144, 0x5,   0x3e,  0x20,  0x8,
        0x144, 0x154,  0x3,    0x2,    0x2,    0x2,    0x145,  0x146,  0x7,    0x29,  0x2,   0x2,   0x146, 0x154, 0x5,   0x3e,
        0x20,  0x7,    0x147,  0x148,  0x7,    0x17,   0x2,    0x2,    0x148,  0x14d, 0x5,   0x3a,  0x1e,  0x2,   0x149, 0x14a,
        0x7,   0x16,   0x2,    0x2,    0x14a,  0x14c,  0x5,    0x3a,   0x1e,   0x2,   0x14b, 0x149, 0x3,   0x2,   0x2,   0x2,
        0x14c, 0x14f,  0x3,    0x2,    0x2,    0x2,    0x14d,  0x14b,  0x3,    0x2,   0x2,   0x2,   0x14d, 0x14e, 0x3,   0x2,
        0x2,   0x2,    0x14e,  0x150,  0x3,    0x2,    0x2,    0x2,    0x14f,  0x14d, 0x3,   0x2,   0x2,   0x2,   0x150, 0x151,
        0x7,   0x18,   0x2,    0x2,    0x151,  0x154,  0x3,    0x2,    0x2,    0x2,   0x152, 0x154, 0x5,   0x36,  0x1c,  0x2,
        0x153, 0x140,  0x3,    0x2,    0x2,    0x2,    0x153,  0x142,  0x3,    0x2,   0x2,   0x2,   0x153, 0x145, 0x3,   0x2,
        0x2,   0x2,    0x153,  0x147,  0x3,    0x2,    0x2,    0x2,    0x153,  0x152, 0x3,   0x2,   0x2,   0x2,   0x154, 0x15f,
        0x3,   0x2,    0x2,    0x2,    0x155,  0x156,  0xc,    0x5,    0x2,    0x2,   0x156, 0x157, 0x5,   0x56,  0x2c,  0x2,
        0x157, 0x158,  0x5,    0x3e,   0x20,   0x6,    0x158,  0x15e,  0x3,    0x2,   0x2,   0x2,   0x159, 0x15a, 0xc,   0x4,
        0x2,   0x2,    0x15a,  0x15b,  0x5,    0x58,   0x2d,   0x2,    0x15b,  0x15c, 0x5,   0x3e,  0x20,  0x5,   0x15c, 0x15e,
        0x3,   0x2,    0x2,    0x2,    0x15d,  0x155,  0x3,    0x2,    0x2,    0x2,   0x15d, 0x159, 0x3,   0x2,   0x2,   0x2,
        0x15e, 0x161,  0x3,    0x2,    0x2,    0x2,    0x15f,  0x15d,  0x3,    0x2,   0x2,   0x2,   0x15f, 0x160, 0x3,   0x2,
        0x2,   0x2,    0x160,  0x3f,   0x3,    0x2,    0x2,    0x2,    0x161,  0x15f, 0x3,   0x2,   0x2,   0x2,   0x162, 0x163,
        0x5,   0x46,   0x24,   0x2,    0x163,  0x164,  0x7,    0x17,   0x2,    0x2,   0x164, 0x165, 0x5,   0x38,  0x1d,  0x2,
        0x165, 0x166,  0x7,    0x18,   0x2,    0x2,    0x166,  0x170,  0x3,    0x2,   0x2,   0x2,   0x167, 0x16a, 0x5,   0x42,
        0x22,  0x2,    0x168,  0x169,  0x7,    0x40,   0x2,    0x2,    0x169,  0x16b, 0x5,   0x48,  0x25,  0x2,   0x16a, 0x168,
        0x3,   0x2,    0x2,    0x2,    0x16a,  0x16b,  0x3,    0x2,    0x2,    0x2,   0x16b, 0x170, 0x3,   0x2,   0x2,   0x2,
        0x16c, 0x16d,  0x7,    0x4d,   0x2,    0x2,    0x16d,  0x16e,  0x7,    0x40,  0x2,   0x2,   0x16e, 0x170, 0x5,   0x48,
        0x25,  0x2,    0x16f,  0x162,  0x3,    0x2,    0x2,    0x2,    0x16f,  0x167, 0x3,   0x2,   0x2,   0x2,   0x16f, 0x16c,
        0x3,   0x2,    0x2,    0x2,    0x170,  0x41,   0x3,    0x2,    0x2,    0x2,   0x171, 0x172, 0x7,   0x4d,  0x2,   0x2,
        0x172, 0x174,  0x7,    0x22,   0x2,    0x2,    0x173,  0x175,  0x5,    0x44,  0x23,  0x2,   0x174, 0x173, 0x3,   0x2,
        0x2,   0x2,    0x174,  0x175,  0x3,    0x2,    0x2,    0x2,    0x175,  0x176, 0x3,   0x2,   0x2,   0x2,   0x176, 0x177,
        0x7,   0x23,   0x2,    0x2,    0x177,  0x43,   0x3,    0x2,    0x2,    0x2,   0x178, 0x179, 0x5,   0x3e,  0x20,  0x2,
        0x179, 0x17a,  0x5,    0x58,   0x2d,   0x2,    0x17a,  0x17b,  0x5,    0x3e,  0x20,  0x2,   0x17b, 0x182, 0x3,   0x2,
        0x2,   0x2,    0x17c,  0x17f,  0x5,    0x36,   0x1c,   0x2,    0x17d,  0x17e, 0x7,   0x21,  0x2,   0x2,   0x17e, 0x180,
        0x5,   0x36,   0x1c,   0x2,    0x17f,  0x17d,  0x3,    0x2,    0x2,    0x2,   0x17f, 0x180, 0x3,   0x2,   0x2,   0x2,
        0x180, 0x182,  0x3,    0x2,    0x2,    0x2,    0x181,  0x178,  0x3,    0x2,   0x2,   0x2,   0x181, 0x17c, 0x3,   0x2,
        0x2,   0x2,    0x182,  0x45,   0x3,    0x2,    0x2,    0x2,    0x183,  0x18a, 0x3,   0x2,   0x2,   0x2,   0x184, 0x18a,
        0x7,   0x42,   0x2,    0x2,    0x185,  0x18a,  0x7,    0x43,   0x2,    0x2,   0x186, 0x18a, 0x7,   0x44,  0x2,   0x2,
        0x187, 0x18a,  0x7,    0x45,   0x2,    0x2,    0x188,  0x18a,  0x7,    0x46,  0x2,   0x2,   0x189, 0x183, 0x3,   0x2,
        0x2,   0x2,    0x189,  0x184,  0x3,    0x2,    0x2,    0x2,    0x189,  0x185, 0x3,   0x2,   0x2,   0x2,   0x189, 0x186,
        0x3,   0x2,    0x2,    0x2,    0x189,  0x187,  0x3,    0x2,    0x2,    0x2,   0x189, 0x188, 0x3,   0x2,   0x2,   0x2,
        0x18a, 0x47,   0x3,    0x2,    0x2,    0x2,    0x18b,  0x18c,  0x7,    0x4d,  0x2,   0x2,   0x18c, 0x49,  0x3,   0x2,
        0x2,   0x2,    0x18d,  0x18e,  0x7,    0x47,   0x2,    0x2,    0x18e,  0x194, 0x5,   0x4e,  0x28,  0x2,   0x18f, 0x194,
        0x5,   0x40,   0x21,   0x2,    0x190,  0x194,  0x5,    0x22,   0x12,   0x2,   0x191, 0x194, 0x5,   0x3a,  0x1e,  0x2,
        0x192, 0x194,  0x5,    0x4c,   0x27,   0x2,    0x193,  0x18d,  0x3,    0x2,   0x2,   0x2,   0x193, 0x18f, 0x3,   0x2,
        0x2,   0x2,    0x193,  0x190,  0x3,    0x2,    0x2,    0x2,    0x193,  0x191, 0x3,   0x2,   0x2,   0x2,   0x193, 0x192,
        0x3,   0x2,    0x2,    0x2,    0x194,  0x4b,   0x3,    0x2,    0x2,    0x2,   0x195, 0x196, 0x9,   0x6,   0x2,   0x2,
        0x196, 0x4d,   0x3,    0x2,    0x2,    0x2,    0x197,  0x198,  0x7,    0x17,  0x2,   0x2,   0x198, 0x199, 0x5,   0x3a,
        0x1e,  0x2,    0x199,  0x19a,  0x7,    0x16,   0x2,    0x2,    0x19a,  0x19b, 0x5,   0x4a,  0x26,  0x2,   0x19b, 0x19c,
        0x7,   0x16,   0x2,    0x2,    0x19c,  0x19d,  0x5,    0x4a,   0x26,   0x2,   0x19d, 0x19e, 0x7,   0x18,  0x2,   0x2,
        0x19e, 0x4f,   0x3,    0x2,    0x2,    0x2,    0x19f,  0x1a0,  0x9,    0x7,   0x2,   0x2,   0x1a0, 0x51,  0x3,   0x2,
        0x2,   0x2,    0x1a1,  0x1a2,  0x7,    0x36,   0x2,    0x2,    0x1a2,  0x1b2, 0x7,   0x36,  0x2,   0x2,   0x1a3, 0x1b2,
        0x7,   0x4,    0x2,    0x2,    0x1a4,  0x1b2,  0x7,    0x5,    0x2,    0x2,   0x1a5, 0x1a6, 0x7,   0x5,   0x2,   0x2,
        0x1a6, 0x1b2,  0x7,    0x36,   0x2,    0x2,    0x1a7,  0x1a8,  0x7,    0x4,   0x2,   0x2,   0x1a8, 0x1b2, 0x7,   0x36,
        0x2,   0x2,    0x1a9,  0x1aa,  0x7,    0x5,    0x2,    0x2,    0x1aa,  0x1b2, 0x7,   0x4,   0x2,   0x2,   0x1ab, 0x1ac,
        0x7,   0x1a,   0x2,    0x2,    0x1ac,  0x1b2,  0x7,    0x36,   0x2,    0x2,   0x1ad, 0x1ae, 0x7,   0x5,   0x2,   0x2,
        0x1ae, 0x1af,  0x7,    0x36,   0x2,    0x2,    0x1af,  0x1b2,  0x7,    0x4,   0x2,   0x2,   0x1b0, 0x1b2, 0x7,   0x36,
        0x2,   0x2,    0x1b1,  0x1a1,  0x3,    0x2,    0x2,    0x2,    0x1b1,  0x1a3, 0x3,   0x2,   0x2,   0x2,   0x1b1, 0x1a4,
        0x3,   0x2,    0x2,    0x2,    0x1b1,  0x1a5,  0x3,    0x2,    0x2,    0x2,   0x1b1, 0x1a7, 0x3,   0x2,   0x2,   0x2,
        0x1b1, 0x1a9,  0x3,    0x2,    0x2,    0x2,    0x1b1,  0x1ab,  0x3,    0x2,   0x2,   0x2,   0x1b1, 0x1ad, 0x3,   0x2,
        0x2,   0x2,    0x1b1,  0x1b0,  0x3,    0x2,    0x2,    0x2,    0x1b2,  0x53,  0x3,   0x2,   0x2,   0x2,   0x1b3, 0x1b4,
        0x9,   0x8,    0x2,    0x2,    0x1b4,  0x55,   0x3,    0x2,    0x2,    0x2,   0x1b5, 0x1b6, 0x7,   0x5,   0x2,   0x2,
        0x1b6, 0x1bd,  0x7,    0x5,    0x2,    0x2,    0x1b7,  0x1b8,  0x7,    0x4,   0x2,   0x2,   0x1b8, 0x1bd, 0x7,   0x4,
        0x2,   0x2,    0x1b9,  0x1bd,  0x7,    0x6,    0x2,    0x2,    0x1ba,  0x1bd, 0x7,   0x4a,  0x2,   0x2,   0x1bb, 0x1bd,
        0x7,   0x7,    0x2,    0x2,    0x1bc,  0x1b5,  0x3,    0x2,    0x2,    0x2,   0x1bc, 0x1b7, 0x3,   0x2,   0x2,   0x2,
        0x1bc, 0x1b9,  0x3,    0x2,    0x2,    0x2,    0x1bc,  0x1ba,  0x3,    0x2,   0x2,   0x2,   0x1bc, 0x1bb, 0x3,   0x2,
        0x2,   0x2,    0x1bd,  0x57,   0x3,    0x2,    0x2,    0x2,    0x1be,  0x1bf, 0x9,   0x9,   0x2,   0x2,   0x1bf, 0x59,
        0x3,   0x2,    0x2,    0x2,    0x2e,   0x5d,   0x69,   0x6d,   0x71,   0x75,  0x7f,  0x85,  0xa0,  0xae,  0xbb,  0xbf,
        0xc3,  0xc9,   0xcd,   0xd3,   0xd7,   0xdc,   0xe3,   0xea,   0xf1,   0xf5,  0xfa,  0xff,  0x108, 0x10f, 0x118, 0x11d,
        0x125, 0x131,  0x13b,  0x13d,  0x14d,  0x153,  0x15d,  0x15f,  0x16a,  0x16f, 0x174, 0x17f, 0x181, 0x189, 0x193, 0x1b1,
        0x1bc,
    };

    _serializedATN.insert(_serializedATN.end(),
                          serializedATNSegment0,
                          serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));

    atn::ATNDeserializer deserializer;
    _atn = deserializer.deserialize(_serializedATN);

    size_t count = _atn.getNumberOfDecisions();
    _decisionToDFA.reserve(count);
    for (size_t i = 0; i < count; i++) {
        _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
    }
}

xCEPParser::Initializer xCEPParser::_init;
