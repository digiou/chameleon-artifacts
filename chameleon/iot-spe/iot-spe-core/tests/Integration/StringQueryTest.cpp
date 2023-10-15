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

#include <Util/Logger/Logger.hpp>
#include <Util/TestHarxs/TestHarxs.hpp>

#include <API/Schema.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy-dtor"
#include <BaseIntegrationTest.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#pragma clang diagnostic pop
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Common/ExecutableType/Array.hpp>

namespace x {

class StringQueryTest : public Testing::BaseIntegrationTest {
  public:
    /// Return the pointer to instance of Schema.
    template<std::size_t s>
    static auto schemaPointer() noexcept -> SchemaPtr {
        return Schema::create()
            ->addField("key", DataTypeFactory::createUInt32())
            ->addField("value", DataTypeFactory::createFixedChar(s));
    }

    /// Return the pointer to instance of Schema.
    static auto intSchemaPointer() noexcept -> SchemaPtr {
        return Schema::create()
            ->addField("key", DataTypeFactory::createUInt32())
            ->addField("value", DataTypeFactory::createUInt32());
    }

    /// Schema format used throughout the string query tests.
    struct IntSchemaClass {
        uint32_t key, value;
        constexpr auto operator==(IntSchemaClass const& rhs) const noexcept -> bool {
            return key == rhs.key && value == rhs.value;
        }
    };

    /// Schema format used throughout the string query tests which consists of an uint key and a
    /// fixed-size char array.
    template<std::size_t s, typename = std::enable_if_t<s != 0>>
    struct SchemaClass {
        uint32_t key;
        char value[s];

        SchemaClass() = default;

        inline SchemaClass(uint32_t key, std::string const& str) : key(key), value() {
            auto const* value = str.c_str();

            if (auto const pSize = str.size() + 1; pSize != s) {
                throw std::runtime_error("Schema constructed from string of incorrect size.");
            }

            for (std::size_t i = 0; i < s; ++i) {
                this->value[i] = value[i];
            }
        }

        /// Compare the key and all value entries.
        constexpr auto operator==(SchemaClass<s> const& rhs) const noexcept -> bool {
            return key == rhs.key && this->compare(rhs);
        }

        /// Constexpr loop which compares all the values making use of short-circuit evaluation.
        template<std::size_t i = s - 1,
                 typename = std::enable_if_t<i<s>> [[nodiscard]] constexpr auto compare(SchemaClass<s> const& rhs) const noexcept
                 -> bool {
            if constexpr (i > 0) {
                return (value[i] == rhs.value[i]) && this->template compare<i - 1>(rhs);
            } else {
                return value[i] == rhs.value[i];
            }
        }
    };

    static void SetUpTestCase() noexcept {
        x::Logger::setupLogging("StringQueryTest.log", x::LogLevel::LOG_WARNING);
        x_INFO("Setup StringQuery test class.");
    }

    static void TearDownTestCase() noexcept { x_INFO("Tear down StringQuery test class."); }
};

/// Test that padding has no influence on the actual size of an element in x' implementation.
TEST_F(StringQueryTest, paddingTest) {
    constexpr std::size_t fixedArraySize = 3;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();
    ASSERT_EQ(4 + fixedArraySize, schema->getSchemaSizeInBytes());
}

/// Conduct a comparison between two string attributes.
TEST_F(StringQueryTest, conditionOnAttribute) {
    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("value") == Attribute("value")))";
    TestHarxs testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                                  .addLogicalSource("car", schema)
                                  // add a memory source
                                  .attachWorkerWithMemorySourceToCoordinator("car")
                                  // push two elements to the memory source
                                  .pushElement<Schema_t>({1, "112"}, 2)
                                  .pushElement<Schema_t>({1, "222"}, 2)
                                  .pushElement<Schema_t>({4, "333"}, 2)
                                  .pushElement<Schema_t>({2, "112"}, 2)
                                  .pushElement<Schema_t>({1, "555"}, 2)
                                  .pushElement<Schema_t>({1, "666"}, 2)
                                  .validate()
                                  .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "112"}, {1, "222"}, {4, "333"}, {2, "112"}, {1, "555"}, {1, "666"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Tests not-equal operator. TODO: enable when neq expressions are allowed on legacy operators.
TEST_F(StringQueryTest, DISABLED_neqOnChars) {
    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("value") != "112"))";
    TestHarxs testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                                  .addLogicalSource("car", schema)
                                  // add a memory source
                                  .attachWorkerWithMemorySourceToCoordinator("car")
                                  // push two elements to the memory source
                                  .pushElement<Schema_t>({1, "112"}, 2)
                                  .pushElement<Schema_t>({1, "222"}, 2)
                                  .pushElement<Schema_t>({4, "333"}, 2)
                                  .pushElement<Schema_t>({2, "112"}, 2)
                                  .pushElement<Schema_t>({1, "555"}, 2)
                                  .pushElement<Schema_t>({1, "666"}, 2)
                                  .validate()
                                  .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "222"}, {4, "333"}, {1, "555"}, {1, "666"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Test equality operator and equality comparison conducted by logical representation of the schema.
TEST_F(StringQueryTest, eqOnCharsMultipleReturn) {
    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("value") == "112"))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", schema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Schema_t>({1, "112"}, 2)
                           .pushElement<Schema_t>({1, "222"}, 2)
                           .pushElement<Schema_t>({4, "333"}, 2)
                           .pushElement<Schema_t>({2, "112"}, 2)
                           .pushElement<Schema_t>({1, "555"}, 2)
                           .pushElement<Schema_t>({1, "666"}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "112"}, {2, "112"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Test equality operator: Set up a query which matches the data's attribute to a fixed string.
/// The filter allows only a single attribute. Test the query's output.
TEST_F(StringQueryTest, eqOnString) {
    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("value") == std::string("113")))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", schema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Schema_t>({1, "113"}, 2)
                           .pushElement<Schema_t>({1, "222"}, 2)
                           .pushElement<Schema_t>({4, "333"}, 2)
                           .pushElement<Schema_t>({1, "444"}, 2)
                           .pushElement<Schema_t>({1, "555"}, 2)
                           .pushElement<Schema_t>({1, "666"}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "113"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Check that wrong expected result reports a failure with the defined string comparator.
TEST_F(StringQueryTest, stringComparisonFilterOnIntNotComparator) {

    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("key") < 4))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", schema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Schema_t>({1, "111"}, 2)
                           .pushElement<Schema_t>({1, "222"}, 2)
                           .pushElement<Schema_t>({4, "333"}, 2)
                           .pushElement<Schema_t>({1, "444"}, 2)
                           .pushElement<Schema_t>({1, "555"}, 2)
                           .pushElement<Schema_t>({1, "666"}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "111"}, {1, "222"}, {1, "444"}, {1, "555"}, {1, "667"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::Not(::testing::UnorderedElementsAreArray(expectedOutput)));
}

/// Test a query on a schema which contains a fixed-size char array. A filter predicate is evaluated on a value that
/// is not of type non-fixed-size char.
TEST_F(StringQueryTest, stringComparisonFilterOnInt) {

    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("key") < 4))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", schema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Schema_t>({1, "111"}, 2)
                           .pushElement<Schema_t>({1, "222"}, 2)
                           .pushElement<Schema_t>({4, "333"}, 2)
                           .pushElement<Schema_t>({1, "444"}, 2)
                           .pushElement<Schema_t>({1, "555"}, 2)
                           .pushElement<Schema_t>({1, "666"}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "111"}, {1, "222"}, {1, "444"}, {1, "555"}, {1, "666"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Test a query on a schema which contains a fixed-size char array. A filter predicate is evaluated on a value that
/// is not of type non-fixed-size char. The values of the string type are the same.
TEST_F(StringQueryTest, stringComparisonFilterOnIntSame) {

    constexpr auto fixedArraySize = 4;

    using Schema_t = StringQueryTest::SchemaClass<fixedArraySize>;
    auto const schema = StringQueryTest::schemaPointer<fixedArraySize>();

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("key") < 4))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", schema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Schema_t>({1, "234"}, 2)
                           .pushElement<Schema_t>({1, "234"}, 2)
                           .pushElement<Schema_t>({4, "234"}, 2)
                           .pushElement<Schema_t>({1, "234"}, 2)
                           .pushElement<Schema_t>({1, "234"}, 2)
                           .pushElement<Schema_t>({1, "234"}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Schema_t> expectedOutput{{1, "234"}, {1, "234"}, {1, "234"}, {1, "234"}, {1, "234"}};
    std::vector<Schema_t> actualOutput = testHarxs.getOutput<Schema_t>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

/// Test utilizing a single struct which defix equality operator and data schema.
TEST_F(StringQueryTest, testHarxsUtilizeOxtruct) {
    using Car = IntSchemaClass;
    auto const carSchema = intSchemaPointer();

    ASSERT_EQ(8U, carSchema->getSchemaSizeInBytes());

    std::string queryWithFilterOperator = R"(Query::from("car").filter(Attribute("key") < 4))";
    auto testHarxs = TestHarxs(queryWithFilterOperator, *restPort, *rpcCoordinatorPort, getTestResourceFolder())
                           .addLogicalSource("car", carSchema)
                           // add a memory source
                           .attachWorkerWithMemorySourceToCoordinator("car")
                           // push two elements to the memory source
                           .pushElement<Car>({1, 2}, 2)
                           .pushElement<Car>({1, 4}, 2)
                           .pushElement<Car>({4, 3}, 2)
                           .pushElement<Car>({1, 9}, 2)
                           .pushElement<Car>({1, 8}, 2)
                           .pushElement<Car>({1, 9}, 2)
                           .validate()
                           .setupTopology();

    ASSERT_EQ(testHarxs.getWorkerCount(), 1U);

    std::vector<Car> expectedOutput = {{1, 2}, {1, 4}, {1, 9}, {1, 8}, {1, 9}};
    std::vector<Car> actualOutput = testHarxs.getOutput<Car>(expectedOutput.size(), "BottomUp", "NONE", "IN_MEMORY");

    EXPECT_EQ(actualOutput.size(), expectedOutput.size());
    EXPECT_THAT(actualOutput, ::testing::UnorderedElementsAreArray(expectedOutput));
}

}// namespace x
