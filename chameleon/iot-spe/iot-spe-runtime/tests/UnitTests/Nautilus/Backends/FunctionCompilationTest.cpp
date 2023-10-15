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

#include <BaseIntegrationTest.hpp>
#include <Nautilus/Backends/BCInterpreter/ByteCode.hpp>
#include <Nautilus/Interface/DataTypes/MemRef.hpp>
#include <Nautilus/Interface/DataTypes/Value.hpp>
#include <Nautilus/Interface/FunctionCall.hpp>
#include <Nautilus/Tracing/Trace/ExecutionTrace.hpp>
#include <Nautilus/Tracing/TraceContext.hpp>
#include <Runtime/BufferManager.hpp>
#include <TestUtils/AbstractCompilationBackendTest.hpp>
#include <Util/Logger/Logger.hpp>
#include <memory>

namespace x::Nautilus {

class FunctionCompilationTest : public Testing::BaseUnitTest, public AbstractCompilationBackendTest {
  public:
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("FunctionCompilationTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("Setup FunctionCompilationTest test class.");
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_INFO("Tear down TraceTest test class."); }
};

int64_t addInt(int64_t x, int64_t y) { return x + y; };

Value<> addIntFunction() {
    auto x = Value<Int64>(2_s64);
    auto y = Value<Int64>(3_s64);
    Value<Int64> res = FunctionCall<>("add", addInt, x, y);
    return res;
}

TEST_P(FunctionCompilationTest, addIntFunctionTest) {

    auto executionTrace = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return addIntFunction();
    });
    auto result = prepare(executionTrace);
    auto function = result->getInvocableMember<int64_t>("execute");
    ASSERT_EQ(function(), 5);
}

int64_t returnConst() { return 42; };

Value<> returnConstFunction() { return FunctionCall<>("returnConst", returnConst); }

TEST_P(FunctionCompilationTest, returnConstFunctionTest) {

    auto executionTrace = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return returnConstFunction();
    });
    auto result = prepare(executionTrace);
    auto function = result->getInvocableMember<int64_t>("execute");
    ASSERT_EQ(function(), 42);
}

void voidException() { x_THROW_RUNTIME_ERROR("An expected exception"); };

void voidExceptionFunction() { FunctionCall<>("voidException", voidException); }

TEST_P(FunctionCompilationTest, voidExceptionFunctionTest) {
    if (this->GetParam() != "MLIR") {
        // Only MLIR supports exceptions
        GTEST_SKIP();
    }
    auto executionTrace = Nautilus::Tracing::traceFunction([]() {
        voidExceptionFunction();
    });
    auto result = prepare(executionTrace);
    auto function = result->getInvocableMember<int64_t>("execute");
    ASSERT_ANY_THROW(function());
}

int64_t multiplyArgument(int64_t x) { return x * 10; };

Value<> multiplyArgumentFunction(Value<Int64> x) {
    Value<Int64> res = FunctionCall<>("multiplyArgument", multiplyArgument, x);
    return res;
}

TEST_P(FunctionCompilationTest, multiplyArgumentTest) {
    Value<Int64> tempPara = +0_s64;
    tempPara.ref = Nautilus::Tracing::ValueRef(INT32_MAX, 0, IR::Types::StampFactory::createInt64Stamp());
    auto executionTrace = Nautilus::Tracing::traceFunctionWithReturn([&tempPara]() {
        return multiplyArgumentFunction(tempPara);
    });
    auto result = prepare(executionTrace);
    auto function = result->getInvocableMember<int64_t, int64_t>("execute");
    ASSERT_EQ(function(10), 100);
    ASSERT_EQ(function(42), 420);
}

// Tests all registered compilation backends.
// To select a specific compilation backend use ::testing::Values("MLIR") instead of ValuesIn.
INSTANTIATE_TEST_CASE_P(testFunctionCalls,
                        FunctionCompilationTest,
                        ::testing::ValuesIn(Backends::CompilationBackendRegistry::getPluginNames().begin(),
                                            Backends::CompilationBackendRegistry::getPluginNames().end()),
                        [](const testing::TestParamInfo<FunctionCompilationTest::ParamType>& info) {
                            return info.param;
                        });

}// namespace x::Nautilus