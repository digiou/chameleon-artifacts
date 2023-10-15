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
#include <Nautilus/Interface/DataTypes/MemRef.hpp>
#include <Nautilus/Interface/DataTypes/Value.hpp>
#include <Nautilus/Tracing/TraceContext.hpp>
#include <Runtime/BufferManager.hpp>
#include <TestUtils/AbstractCompilationBackendTest.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>
#include <memory>

using namespace x::Nautilus;
namespace x::Nautilus {

class LoopCompilationTest : public Testing::BaseUnitTest, public AbstractCompilationBackendTest {
  public:
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("TraceTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("Setup TraceTest test class.");
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_DEBUG("Tear down TraceTest test class."); }
};

Value<> sumLoop(int upperLimit) {
    Value agg = Value(1);
    for (Value start = 0; start < upperLimit; start = start + 1) {
        agg = agg + 10;
    }
    return agg;
}

TEST_P(LoopCompilationTest, sumLoopTestSCF) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return sumLoop(10);
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 101);
}

Value<> xtedSumLoop(int upperLimit) {
    Value agg = Value(1);
    for (Value start = 0; start < upperLimit; start = start + 1) {
        for (Value start2 = 0; start2 < upperLimit; start2 = start2 + 1) {
            agg = agg + 10;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, xtedLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return xtedSumLoop(10);
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 1001);
}

TEST_P(LoopCompilationTest, sumLoopTestCF) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return sumLoop(10);
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 101);
}

Value<> ifSumLoop() {
    Value agg = Value(1);
    for (Value start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
            agg = agg + 10;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, ifSumLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return ifSumLoop();
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 51);
}

Value<> ifElseSumLoop() {
    Value agg = Value(1);
    for (Value<Int32> start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
            agg = agg + 10;
        } else {
            agg = agg + 1;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, ifElseSumLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return ifElseSumLoop();
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 56);
}

Value<> elseOnlySumLoop() {
    Value agg = Value(1);
    for (Value start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
        } else {
            agg = agg + 1;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, elseOnlySumLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return elseOnlySumLoop();
    });

    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 1);
}

Value<> xtedIfSumLoop() {
    Value agg = Value(1);
    for (Value start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
            if (agg < 40) {
                agg = agg + 10;
            }
        } else {
            agg = agg + 1;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, xtedIfSumLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return xtedIfSumLoop();
    });
    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 41);
}

Value<> xtedIfElseSumLoop() {
    Value agg = Value(1);
    for (Value start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
            if (agg < 40) {
                agg = agg + 10;
            } else {
                agg = agg + 100;
            }
        } else {
            agg = agg + 1;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, xtedIfElseSumLoopTest) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return xtedIfElseSumLoop();
    });
    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 146);
}

Value<> xtedElseOnlySumLoop() {
    Value agg = Value(1);
    for (Value start = 0; start < 10; start = start + 1) {
        if (agg < 50) {
            if (agg < 40) {
            } else {
                agg = agg + 100;
            }
        } else {
            agg = agg + 1;
        }
    }
    return agg;
}

TEST_P(LoopCompilationTest, xtedElseOnlySumLoop) {
    auto execution = Nautilus::Tracing::traceFunctionWithReturn([]() {
        return xtedElseOnlySumLoop();
    });
    auto engine = prepare(execution);
    auto function = engine->getInvocableMember<int32_t>("execute");
    ASSERT_EQ(function(), 1);
}

// Tests all registered compilation backends.
// To select a specific compilation backend use ::testing::Values("MLIR") instead of ValuesIn.
INSTANTIATE_TEST_CASE_P(testLoopCompilation,
                        LoopCompilationTest,
                        ::testing::ValuesIn(Backends::CompilationBackendRegistry::getPluginNames().begin(),
                                            Backends::CompilationBackendRegistry::getPluginNames().end()),
                        [](const testing::TestParamInfo<LoopCompilationTest::ParamType>& info) {
                            return info.param;
                        });

}// namespace x::Nautilus