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

#ifdef ENABLE_JNI

#include <API/Schema.hpp>
#include <BaseIntegrationTest.hpp>
#include <Catalogs/UDF/JavaUDFDescriptor.hpp>
#include <Util/JavaUDFDescriptorBuilder.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestExecutionEngine.hpp>
#include <Util/TestSinkDescriptor.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <iostream>
#include <jni.h>
#include <utility>

using namespace x;
using Runtime::TupleBuffer;

class FlatMapJavaUDFQueryExecutionTest : public Testing::BaseUnitTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("FlatMapJavaUDFQueryExecutionTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("QueryExecutionTest: Setup FlatMapJavaUDFQueryExecutionTest test class.");
    }
    /* Will be called before a test is executed. */
    void SetUp() override {
        Testing::BaseUnitTest::SetUp();
        executionEngine = std::make_shared<x::Testing::TestExecutionEngine>(
            QueryCompilation::QueryCompilerOptions::QueryCompiler::NAUTILUS_QUERY_COMPILER);
    }

    /* Will be called before a test is executed. */
    void TearDown() override {
        Testing::BaseUnitTest::TearDown();
        x_DEBUG("QueryExecutionTest: Tear down FlatMapJavaUDFQueryExecutionTest test case.");
        ASSERT_TRUE(executionEngine->stop());
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_DEBUG("FlatMapJavaUDFQueryExecutionTest: Tear down QueryExecutionTest test class."); }

    std::shared_ptr<x::Testing::TestExecutionEngine> executionEngine;
    std::string testDataPath = std::string(TEST_DATA_DIRECTORY) + "/JavaUDFTestData/";
};

constexpr auto NUMBER_OF_RECORDS = 10;
constexpr auto UDF_INCREMENT = 10;

/**
 * This helper function fills a buffer with test data
 */
void fillBuffer(Runtime::MemoryLayouts::DynamicTupleBuffer& buf) {
    for (int recordIndex = 0; recordIndex < NUMBER_OF_RECORDS; recordIndex++) {
        buf[recordIndex][0].write<int32_t>(recordIndex);
    }
    buf.setNumberOfTuples(NUMBER_OF_RECORDS);
}

/**
 * This helper function creates a JVM and returns the JNIEnv.
 */
void createJVM(JavaVM* jvm, JNIEnv** env) {
    JavaVMInitArgs args{};
    std::vector<std::string> opt{"-verbose:jni", "-verbose:class"};
    std::vector<JavaVMOption> options;
    for (const auto& s : opt) {
        options.push_back(JavaVMOption{.optionString = const_cast<char*>(s.c_str())});
    }
    args.version = JNI_VERSION_1_8;
    args.ignoreUnrecognized = false;
    args.options = options.data();
    args.nOptions = std::size(options);
    JNI_CreateJavaVM(&jvm, (void**) env, &args);
}

/**
 * This helper function loads the class files into a buffer.
 */
std::vector<char> loadClassFileIntoBuffer(const std::string& path, const std::string& className) {
    // Open the file
    std::ifstream file(path + className + ".class", std::ios::binary);
    if (!file.is_open()) {
        x_ERROR("Could not open file: {}", path + className + ".class");
        return {};
    }

    // Get the file size and allocate a buffer
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    std::vector<char> buffer(fileSize);

    // Read the file into the buffer
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), fileSize)) {
        x_ERROR("Could not read file: {}", path + className + ".class");
        return {};
    }
    return buffer;
}

/**
 * @brief Test simple UDF with integer objects as input and output (IntegerMapFunction<Integer, Integer>)
 * The UDF increments incoming tuples by its current state starting from 10 and
*/
TEST_F(FlatMapJavaUDFQueryExecutionTest, FlatMapJavaUdf) {
    auto schema = Schema::create()->addField("id", BasicType::INT32);
    auto testSink = executionEngine->createDataSink(schema);
    auto testSourceDescriptor = executionEngine->createDataSource(schema);

    std::vector<std::string> classNames = {"stream/IoTSPE/FlatMapFunction", "IntegerFlatMapFunction"};
    auto methodName = "flatMap";
    std::vector<char> serializedInstance = {};
    jni::JavaUDFByteCodeList byteCodeList;
    for (const auto& className : classNames) {
        auto buffer = loadClassFileIntoBuffer(testDataPath, className);
        byteCodeList.emplace_back(className, buffer);
    }

    auto className = classNames[1];
    auto outputSchema = Schema::create()->addField("id", BasicType::INT32);
    x_INFO("testDataPath:{}", testDataPath);
    auto javaUDFDescriptor = Catalogs::UDF::JavaUDFDescriptorBuilder{}
                                 .setClassName(className)
                                 .setMethodName(methodName)
                                 .setInstance(serializedInstance)
                                 .setByteCodeList(byteCodeList)
                                 .setOutputSchema(outputSchema)
                                 .setInputClassName("java.lang.Integer")
                                 .setOutputClassName("java.util.Collection")
                                 .build();
    auto testSinkDescriptor = std::make_shared<TestUtils::TestSinkDescriptor>(testSink);
    auto query = TestQuery::from(testSourceDescriptor).flatMapUDF(javaUDFDescriptor).sink(testSinkDescriptor);
    auto plan = executionEngine->submitQuery(query.getQueryPlan());
    auto source = executionEngine->getDataSource(plan, 0);
    ASSERT_TRUE(!!source);
    auto inputBuffer = executionEngine->getBuffer(schema);
    fillBuffer(inputBuffer);
    source->emitBuffer(inputBuffer);
    testSink->waitTillCompleted();
    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 1u);
    auto resultBuffer = testSink->getResultBuffer(0);

    EXPECT_EQ(resultBuffer.getNumberOfTuples(), NUMBER_OF_RECORDS);
    for (uint32_t recordIndex = 0u; recordIndex < NUMBER_OF_RECORDS; ++recordIndex) {
        EXPECT_EQ(resultBuffer[recordIndex][0].read<int32_t>(), recordIndex + UDF_INCREMENT);
    }
    ASSERT_TRUE(executionEngine->stopQuery(plan));
    ASSERT_EQ(testSink->getNumberOfResultBuffers(), 0U);
}

#endif// ENABLE_JNI