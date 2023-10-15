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
#include <Util/Common.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>

namespace x {
class UtilFunctionTest : public Testing::BaseUnitTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("UtilFunctionTest.log", x::LogLevel::LOG_DEBUG);

        x_INFO("UtilFunctionTest test class SetUpTestCase.");
    }
    static void TearDownTestCase() { x_INFO("UtilFunctionTest test class TearDownTestCase."); }
};

TEST(UtilFunctionTest, replaceNothing) {
    std::string origin = "I do not have the search string in me.";
    std::string search = "IoTSPE";
    std::string replace = "replacing";
    std::string replacedString = Util::replaceFirst(origin, search, replace);
    EXPECT_TRUE(replacedString == origin);
}

TEST(UtilFunctionTest, replaceOnceWithOneFinding) {
    std::string origin = "I do  have the search string IoTSPE in me, but only once.";
    std::string search = "IoTSPE";
    std::string replace = "replacing";
    std::string replacedString = Util::replaceFirst(origin, search, replace);
    std::string expectedReplacedString = "I do  have the search string replacing in me, but only once.";
    EXPECT_TRUE(replacedString == expectedReplacedString);
}

TEST(UtilFunctionTest, replaceOnceWithMultipleFindings) {
    std::string origin = "I do  have the search string IoTSPE in me, but multiple times IoTSPE";
    std::string search = "IoTSPE";
    std::string replace = "replacing";
    std::string replacedString = Util::replaceFirst(origin, search, replace);
    std::string expectedReplacedString = "I do  have the search string replacing in me, but multiple times IoTSPE";
    EXPECT_TRUE(replacedString == expectedReplacedString);
}

TEST(UtilFunctionTest, splitWithStringDelimiterNothing) {
    std::vector<std::string> tokens;
    std::vector<std::string> test;
    test.emplace_back("This is a random test line with no delimiter.");
    std::string line = "This is a random test line with no delimiter.";
    std::string delimiter = "x";
    tokens = x::Util::splitWithStringDelimiter<std::string>(line, delimiter);
    EXPECT_TRUE(tokens == test);
}

TEST(UtilFunctionTest, splitWithStringDelimiterOnce) {
    std::vector<std::string> tokens;
    std::vector<std::string> test;
    test.emplace_back("This is a random test line with ");
    test.emplace_back(" delimiter.");
    std::string line = "This is a random test line with x delimiter.";
    std::string delimiter = "x";
    tokens = x::Util::splitWithStringDelimiter<std::string>(line, delimiter);
    EXPECT_TRUE(tokens == test);
}

TEST(UtilFunctionTest, splitWithStringDelimiterTwice) {
    std::vector<std::string> tokens;
    std::vector<std::string> test;
    test.emplace_back("This is a random ");
    test.emplace_back(" line with ");
    test.emplace_back(" delimiter.");
    std::string line = "This is a random x line with x delimiter.";
    std::string delimiter = "x";
    tokens = x::Util::splitWithStringDelimiter<std::string>(line, delimiter);
    EXPECT_TRUE(tokens == test);
}

TEST(UtilFunctionTest, splitWithOmittingEmptyLast) {
    std::vector<std::string> tokens;
    std::vector<std::string> test;
    test.emplace_back("This is a random ");
    test.emplace_back(" line with ");
    test.emplace_back(" delimiter. ");
    std::string line = "This is a random x line with x delimiter. x";
    std::string delimiter = "x";
    tokens = x::Util::splitWithStringDelimiter<std::string>(line, delimiter);
    EXPECT_TRUE(tokens == test);
}

}// namespace x