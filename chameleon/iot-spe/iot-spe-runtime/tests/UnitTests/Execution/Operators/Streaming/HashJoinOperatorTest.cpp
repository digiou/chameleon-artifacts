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

#include <API/Schema.hpp>
#include <BaseIntegrationTest.hpp>
#include <Common/DataTypes/DataType.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Exceptions/ErrorListener.hpp>
#include <Execution/Expressions/ReadFieldExpression.hpp>
#include <Execution/Operators/ExecutionContext.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/JoinPhases/StreamHashJoinBuild.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/JoinPhases/StreamHashJoinProbe.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/StreamHashJoinOperatorHandler.hpp>
#include <Execution/Operators/Streaming/Join/StreamJoinOperatorHandler.hpp>
#include <Execution/Operators/Streaming/TimeFunction.hpp>
#include <Execution/RecordBuffer.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/WorkerContext.hpp>
#include <TestUtils/RecordCollectOperator.hpp>
#include <TestUtils/UtilityFunctions.hpp>
#include <Util/Common.hpp>

namespace x::Runtime::Execution {

class HashJoinOperatorTest : public Testing::BaseUnitTest {
  public:
    std::shared_ptr<Runtime::BufferManager> bm;
    std::vector<TupleBuffer> emittedBuffers;

    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("HashJoinOperatorTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup HashJoinOperatorTest test class.");
    }

    /* Will be called before a test is executed. */
    void SetUp() override {
        BaseUnitTest::SetUp();
        x_INFO("Setup HashJoinOperatorTest test case.");
        bm = std::make_shared<Runtime::BufferManager>();
    }

    /* Will be called after a test is executed. */
    void TearDown() override {
        x_INFO("Tear down HashJoinOperatorTest test case.");
        BaseUnitTest::TearDown();
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { x_INFO("Tear down HashJoinOperatorTest test class."); }
};

struct HashJoinBuildHelper {
    size_t pageSize;
    size_t numPartitions;
    size_t numberOfTuplesToProduce;
    size_t numberOfBuffersPerWorker;
    size_t noWorkerThreads;
    size_t totalNumSources;
    size_t joinSizeInByte;
    size_t windowSize;
    Operators::StreamHashJoinBuildPtr hashJoinBuild;
    std::string joinFieldName;
    BufferManagerPtr bufferManager;
    SchemaPtr schema;
    std::string timeStampField;
    HashJoinOperatorTest* hashJoinOperatorTest;
    QueryCompilation::JoinBuildSideType joinBuildSide;

    HashJoinBuildHelper(Operators::StreamHashJoinBuildPtr hashJoinBuild,
                        const std::string& joinFieldName,
                        BufferManagerPtr bufferManager,
                        SchemaPtr schema,
                        const std::string& timeStampField,
                        HashJoinOperatorTest* hashJoinOperatorTest,
                        QueryCompilation::JoinBuildSideType joinBuildSide)
        : pageSize(131072), numPartitions(1), numberOfTuplesToProduce(100), numberOfBuffersPerWorker(128), noWorkerThreads(1),
          totalNumSources(2), joinSizeInByte(1 * 1024 * 1024), windowSize(1000), hashJoinBuild(hashJoinBuild),
          joinFieldName(joinFieldName), bufferManager(bufferManager), schema(schema), timeStampField(timeStampField),
          hashJoinOperatorTest(hashJoinOperatorTest), joinBuildSide(joinBuildSide) {}
};

bool hashJoinBuildAndCheck(HashJoinBuildHelper buildHelper) {
    OriginId outputOriginId = 1;
    auto workerContext =
        std::make_shared<WorkerContext>(/*workerId*/ 0, buildHelper.bufferManager, buildHelper.numberOfBuffersPerWorker);
    auto hashJoinOpHandler =
        Operators::StreamHashJoinOperatorHandler::create(std::vector<::OriginId>({1}),
                                                         outputOriginId,
                                                         buildHelper.windowSize,
                                                         buildHelper.schema->getSchemaSizeInBytes(),
                                                         buildHelper.schema->getSchemaSizeInBytes(),
                                                         buildHelper.joinSizeInByte,
                                                         buildHelper.pageSize,
                                                         1,
                                                         buildHelper.numPartitions,
                                                         QueryCompilation::StreamJoinStrategy::HASH_JOIN_LOCAL);

    auto hashJoinOperatorTest = buildHelper.hashJoinOperatorTest;
    auto pipelineContext = PipelineExecutionContext(
        -1,// mock pipeline id
        0, // mock query id
        nullptr,
        buildHelper.noWorkerThreads,
        [&hashJoinOperatorTest](TupleBuffer& buffer, Runtime::WorkerContextRef) {
            hashJoinOperatorTest->emittedBuffers.emplace_back(std::move(buffer));
        },
        [&hashJoinOperatorTest](TupleBuffer& buffer) {
            hashJoinOperatorTest->emittedBuffers.emplace_back(std::move(buffer));
        },
        {hashJoinOpHandler});

    auto executionContext = ExecutionContext(Nautilus::Value<Nautilus::MemRef>((int8_t*) workerContext.get()),
                                             Nautilus::Value<Nautilus::MemRef>((int8_t*) (&pipelineContext)));

    buildHelper.hashJoinBuild->setup(executionContext);

    // Execute record and thus fill the hash table
    for (auto i = 0UL; i < buildHelper.numberOfTuplesToProduce + 1; ++i) {
        auto record = Nautilus::Record({{buildHelper.schema->get(0)->getName(), Value<UInt64>((uint64_t) i)},
                                        {buildHelper.schema->get(1)->getName(), Value<UInt64>((uint64_t) (i % 10) + 1)},
                                        {buildHelper.schema->get(2)->getName(), Value<UInt64>((uint64_t) i)}});

        if (i == 0) {
            auto tupleBuffer = Util::getBufferFromRecord(record, buildHelper.schema, buildHelper.bufferManager);
            RecordBuffer recordBuffer = RecordBuffer(Value<MemRef>((int8_t*) std::addressof(tupleBuffer)));
            buildHelper.hashJoinBuild->open(executionContext, recordBuffer);
        }

        buildHelper.hashJoinBuild->execute(executionContext, record);

        uint64_t joinKey = record.read(buildHelper.joinFieldName).as<UInt64>().getValue().getValue();
        uint64_t timeStamp = record.read(buildHelper.timeStampField).as<UInt64>().getValue().getValue();
        auto hash = ::x::Util::murmurHash(joinKey);
        auto window = hashJoinOpHandler->getWindowByTimestampOrCreateIt(timeStamp);
        auto hashWindow = static_cast<StreamHashJoinWindow*>(window.get());

        auto hashTable = hashWindow->getHashTable(buildHelper.joinBuildSide, workerContext->getId());

        auto bucket = hashTable->getBucketLinkedList(hashTable->getBucketPos(hash));

        bool correctlyInserted = false;
        for (auto&& page : bucket->getPages()) {
            for (auto k = 0UL; k < page->size(); ++k) {
                uint8_t* recordPtr = page.get()->operator[](k);
                auto bucketBuffer = Util::getBufferFromPointer(recordPtr, buildHelper.schema, buildHelper.bufferManager);
                auto recordBuffer = Util::getBufferFromRecord(record, buildHelper.schema, buildHelper.bufferManager);

                if (memcmp(bucketBuffer.getBuffer(), recordBuffer.getBuffer(), buildHelper.schema->getSchemaSizeInBytes()) == 0) {
                    correctlyInserted = true;
                    break;
                }
            }
        }

        if (!correctlyInserted) {
            auto recordBuffer = Util::getBufferFromRecord(record, buildHelper.schema, buildHelper.bufferManager);
            x_ERROR("Could not find record {} in bucket!", Util::printTupleBufferAsCSV(recordBuffer, buildHelper.schema));
            return false;
        }
    }

    return true;
}

struct HashJoinProbeHelper {
    size_t pageSize;
    size_t numPartitions;
    size_t numberOfTuplesToProduce;
    size_t numberOfBuffersPerWorker;
    size_t noWorkerThreads;
    size_t joinSizeInByte;
    uint64_t windowSize;
    std::string joinFieldNameLeft, joinFieldNameRight;
    BufferManagerPtr bufferManager;
    SchemaPtr leftSchema, rightSchema;
    std::string timeStampFieldLeft;
    std::string timeStampFieldRight;
    HashJoinOperatorTest* hashJoinOperatorTest;

    HashJoinProbeHelper(const std::string& joinFieldNameLeft,
                        const std::string& joinFieldNameRight,
                        BufferManagerPtr bufferManager,
                        SchemaPtr leftSchema,
                        SchemaPtr rightSchema,
                        const std::string& timeStampFieldLeft,
                        const std::string& timeStampFieldRight,
                        HashJoinOperatorTest* hashJoinOperatorTest)
        : pageSize(131072), numPartitions(1), numberOfTuplesToProduce(100), numberOfBuffersPerWorker(128), noWorkerThreads(1),
          joinSizeInByte(1 * 1024 * 1024), windowSize(1000), joinFieldNameLeft(joinFieldNameLeft),
          joinFieldNameRight(joinFieldNameRight), bufferManager(bufferManager), leftSchema(leftSchema), rightSchema(rightSchema),
          timeStampFieldLeft(timeStampFieldLeft), timeStampFieldRight(timeStampFieldRight),
          hashJoinOperatorTest(hashJoinOperatorTest) {}
};

bool checkIfBufferFoundAndRemove(std::vector<Runtime::TupleBuffer>& emittedBuffers,
                                 Runtime::TupleBuffer expectedBuffer,
                                 SchemaPtr joinSchema,
                                 uint64_t& removedBuffer) {

    bool foundBuffer = false;
    x_TRACE("NLJ buffer = {}", Util::printTupleBufferAsCSV(expectedBuffer, joinSchema));

    for (auto tupleBufferIt = emittedBuffers.begin(); tupleBufferIt != emittedBuffers.end(); ++tupleBufferIt) {
        x_TRACE("Comparing versus {}", Util::printTupleBufferAsCSV(*tupleBufferIt, joinSchema));
        if (memcmp(tupleBufferIt->getBuffer(),
                   expectedBuffer.getBuffer(),
                   joinSchema->getSchemaSizeInBytes() * expectedBuffer.getNumberOfTuples())
            == 0) {
            x_TRACE("Removing buffer #{} {} of size {}",
                      removedBuffer,
                      Util::printTupleBufferAsCSV(*tupleBufferIt, joinSchema),
                      joinSchema->getSchemaSizeInBytes());
            emittedBuffers.erase(tupleBufferIt);
            foundBuffer = true;
            removedBuffer += 1;
            break;
        }
    }
    return foundBuffer;
}

uint64_t calculateExpNoTuplesInWindow(uint64_t totalTuples, uint64_t windowIdentifier, uint64_t windowSize) {
    std::vector<uint64_t> tmpVec;
    while (totalTuples > windowSize) {
        tmpVec.emplace_back(windowSize);
        totalTuples -= windowSize;
    }
    tmpVec.emplace_back(totalTuples);
    auto noWindow = (windowIdentifier - 1) / windowSize;
    return tmpVec[noWindow];
}

bool hashJoinProbeAndCheck(HashJoinProbeHelper hashJoinProbeHelper) {

    if (!hashJoinProbeHelper.leftSchema->contains(hashJoinProbeHelper.joinFieldNameLeft)) {
        x_ERROR("JoinFieldNameLeft {} is not in leftSchema!", hashJoinProbeHelper.joinFieldNameLeft);
        return false;
    }
    if (!hashJoinProbeHelper.rightSchema->contains(hashJoinProbeHelper.joinFieldNameRight)) {
        x_ERROR("JoinFieldNameLeft {} is not in leftSchema!", hashJoinProbeHelper.joinFieldNameRight);
        return false;
    }

    auto workerContext = std::make_shared<WorkerContext>(/*workerId*/ 0,
                                                         hashJoinProbeHelper.bufferManager,
                                                         hashJoinProbeHelper.numberOfBuffersPerWorker);
    OriginId outputOriginId = 1;
    auto hashJoinOpHandler =
        Operators::StreamHashJoinOperatorHandler::create(std::vector<::OriginId>({1, 2}),
                                                         outputOriginId,
                                                         hashJoinProbeHelper.windowSize,
                                                         hashJoinProbeHelper.leftSchema->getSchemaSizeInBytes(),
                                                         hashJoinProbeHelper.rightSchema->getSchemaSizeInBytes(),
                                                         hashJoinProbeHelper.joinSizeInByte,
                                                         hashJoinProbeHelper.pageSize,
                                                         1,
                                                         hashJoinProbeHelper.numPartitions,
                                                         QueryCompilation::StreamJoinStrategy::HASH_JOIN_LOCAL);

    auto hashJoinOperatorTest = hashJoinProbeHelper.hashJoinOperatorTest;
    auto pipelineContext = PipelineExecutionContext(
        0,// mock pipeline id
        1,// mock query id
        hashJoinProbeHelper.bufferManager,
        hashJoinProbeHelper.noWorkerThreads,
        [&hashJoinOperatorTest](TupleBuffer& buffer, Runtime::WorkerContextRef) {
            hashJoinOperatorTest->emittedBuffers.emplace_back(std::move(buffer));
        },
        [&hashJoinOperatorTest](TupleBuffer& buffer) {
            hashJoinOperatorTest->emittedBuffers.emplace_back(std::move(buffer));
        },
        {hashJoinOpHandler});

    auto executionContext = ExecutionContext(Nautilus::Value<Nautilus::MemRef>((int8_t*) workerContext.get()),
                                             Nautilus::Value<Nautilus::MemRef>((int8_t*) (&pipelineContext)));

    auto handlerIndex = 0UL;
    auto readTsFieldLeft = std::make_shared<Expressions::ReadFieldExpression>(hashJoinProbeHelper.timeStampFieldLeft);
    auto readTsFieldRight = std::make_shared<Expressions::ReadFieldExpression>(hashJoinProbeHelper.timeStampFieldRight);

    auto hashJoinBuildLeft = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        QueryCompilation::JoinBuildSideType::Left,
        hashJoinProbeHelper.joinFieldNameLeft,
        hashJoinProbeHelper.timeStampFieldLeft,
        hashJoinProbeHelper.leftSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsFieldLeft));
    auto hashJoinBuildRight = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        QueryCompilation::JoinBuildSideType::Right,
        hashJoinProbeHelper.joinFieldNameRight,
        hashJoinProbeHelper.timeStampFieldRight,
        hashJoinProbeHelper.rightSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsFieldRight));

    auto joinSchema = Util::createJoinSchema(hashJoinProbeHelper.leftSchema,
                                             hashJoinProbeHelper.rightSchema,
                                             hashJoinProbeHelper.joinFieldNameLeft);

    auto hashJoinProbe = std::make_shared<Operators::StreamHashJoinProbe>(handlerIndex,
                                                                          hashJoinProbeHelper.leftSchema,
                                                                          hashJoinProbeHelper.rightSchema,
                                                                          joinSchema,
                                                                          hashJoinProbeHelper.joinFieldNameLeft,
                                                                          hashJoinProbeHelper.joinFieldNameRight);
    auto collector = std::make_shared<Operators::CollectOperator>();
    hashJoinProbe->setChild(collector);
    hashJoinProbe->setDeletion(false);
    hashJoinBuildLeft->setup(executionContext);
    hashJoinBuildRight->setup(executionContext);

    std::vector<std::vector<Nautilus::Record>> leftRecords;
    std::vector<std::vector<Nautilus::Record>> rightRecords;

    uint64_t lastTupleTimeStampWindow = hashJoinProbeHelper.windowSize - 1;
    std::vector<Nautilus::Record> tmpRecordsLeft, tmpRecordsRight;

    //create buffers
    for (auto i = 0UL; i < hashJoinProbeHelper.numberOfTuplesToProduce + 1; ++i) {
        auto recordLeft =
            Nautilus::Record({{hashJoinProbeHelper.leftSchema->get(0)->getName(), Value<UInt64>((uint64_t) i)},
                              {hashJoinProbeHelper.leftSchema->get(1)->getName(), Value<UInt64>((uint64_t) (i % 10) + 10)},
                              {hashJoinProbeHelper.leftSchema->get(2)->getName(), Value<UInt64>((uint64_t) i)}});
        x_DEBUG("Tuple left id={} key={} ts={}", i, (i % 10) + 10, i);
        auto recordRight =
            Nautilus::Record({{hashJoinProbeHelper.rightSchema->get(0)->getName(), Value<UInt64>((uint64_t) i + 1000)},
                              {hashJoinProbeHelper.rightSchema->get(1)->getName(), Value<UInt64>((uint64_t) (i % 10) + 10)},
                              {hashJoinProbeHelper.rightSchema->get(2)->getName(), Value<UInt64>((uint64_t) i)}});
        x_DEBUG("Tuple right f1_left={} kef2_left(key)={} ts={}", i + 1000, (i % 10) + 10, i);

        if (recordRight.read(hashJoinProbeHelper.timeStampFieldRight) > lastTupleTimeStampWindow) {
            x_DEBUG("rects={} >= {}",
                      recordRight.read(hashJoinProbeHelper.timeStampFieldRight)->toString(),
                      lastTupleTimeStampWindow);
            leftRecords.push_back(std::vector(tmpRecordsLeft.begin(), tmpRecordsLeft.end()));
            rightRecords.push_back(std::vector(tmpRecordsRight.begin(), tmpRecordsRight.end()));

            tmpRecordsLeft = std::vector<Nautilus::Record>();
            tmpRecordsRight = std::vector<Nautilus::Record>();

            lastTupleTimeStampWindow += hashJoinProbeHelper.windowSize;
        }

        tmpRecordsLeft.emplace_back(recordLeft);
        tmpRecordsRight.emplace_back(recordRight);
    }

    x_DEBUG("filling left side");
    //push buffers to build left
    //for all record buffers
    for (auto i = 0UL; i < leftRecords.size(); i++) {
        auto tupleBuffer = hashJoinProbeHelper.bufferManager->getBufferBlocking();
        RecordBuffer recordBufferLeft = RecordBuffer(Value<MemRef>((int8_t*) std::addressof(tupleBuffer)));
        if (i == 0) {
            hashJoinBuildLeft->open(executionContext, recordBufferLeft);
        }
        uint64_t size = leftRecords[i].size();
        recordBufferLeft.setNumRecords(uint64_t(0));
        //for one record in the buffer
        for (auto& u : leftRecords[i]) {
            hashJoinBuildLeft->execute(executionContext, u);
            x_DEBUG("Insert left tuple {}", u.toString());
        }
        executionContext.setWatermarkTs(leftRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldLeft).as<UInt64>());
        executionContext.setCurrentTs(leftRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldLeft).as<UInt64>());
        executionContext.setOrigin(uint64_t(1));
        executionContext.setSequenceNumber(uint64_t(i));
        x_DEBUG("trigger left with ts={}", leftRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldLeft)->toString());

        hashJoinBuildLeft->close(executionContext, recordBufferLeft);
    }

    x_DEBUG("filling right side with size = {}", rightRecords.size());
    for (auto i = 0UL; i < rightRecords.size(); i++) {
        auto tupleBuffer = hashJoinProbeHelper.bufferManager->getBufferBlocking();
        RecordBuffer recordBufferRight = RecordBuffer(Value<MemRef>((int8_t*) std::addressof(tupleBuffer)));
        if (i == 0) {
            hashJoinBuildRight->open(executionContext, recordBufferRight);
        }
        uint64_t size = rightRecords[i].size();
        recordBufferRight.setNumRecords(uint64_t(0));
        //for one record in the buffer
        for (auto u = 0UL; u < rightRecords[i].size(); u++) {
            hashJoinBuildRight->execute(executionContext, rightRecords[i][u]);
            x_DEBUG("Insert right tuple {}", rightRecords[i][u].toString());
        }
        executionContext.setWatermarkTs(rightRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldRight).as<UInt64>());
        executionContext.setCurrentTs(rightRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldRight).as<UInt64>());
        executionContext.setOrigin(uint64_t(2));
        executionContext.setSequenceNumber(uint64_t(i));
        x_DEBUG("trigger left with ts={}", rightRecords[i][size - 1].read(hashJoinProbeHelper.timeStampFieldRight)->toString());
        hashJoinBuildRight->close(executionContext, recordBufferRight);
    }

    auto numberOfEmittedBuffersBuild = hashJoinOperatorTest->emittedBuffers.size();
    x_DEBUG("trigger Probe for numberOfEmittedBuffersBuild = {}", numberOfEmittedBuffersBuild);
    for (auto cnt = 0UL; cnt < numberOfEmittedBuffersBuild; ++cnt) {
        auto tupleBuffer = hashJoinOperatorTest->emittedBuffers[cnt];
        RecordBuffer recordBuffer = RecordBuffer(Value<MemRef>((int8_t*) std::addressof(tupleBuffer)));
        hashJoinProbe->open(executionContext, recordBuffer);
    }

    // Delete all buffers that have been emitted from the build phase
    hashJoinOperatorTest->emittedBuffers.erase(hashJoinOperatorTest->emittedBuffers.begin(),
                                               hashJoinOperatorTest->emittedBuffers.begin() + numberOfEmittedBuffersBuild);

    /* Checking if all windows have been deleted except for one.
     * We require always one window as we do not know here if we have to take care of more tuples*/
    if (hashJoinOpHandler->as<Operators::StreamJoinOperatorHandler>()->getNumberOfWindows() != 1) {
        x_ERROR("Not exactly one active window! {}",
                  hashJoinOpHandler->as<Operators::StreamJoinOperatorHandler>()->getNumberOfWindows());
        //TODO: this is tricky now we can either activate deletion but then the later code cannot check the window size or we test this here
        //        return false;
    }

    Value<UInt64> zeroValue((uint64_t) 0UL);
    auto maxWindowIdentifier = std::ceil((double) hashJoinProbeHelper.numberOfTuplesToProduce / hashJoinProbeHelper.windowSize)
        * hashJoinProbeHelper.windowSize;
    for (auto windowIdentifier = hashJoinProbeHelper.windowSize; windowIdentifier < maxWindowIdentifier;
         windowIdentifier += hashJoinProbeHelper.windowSize) {
        auto expectedNumberOfTuplesInWindowLeft = calculateExpNoTuplesInWindow(hashJoinProbeHelper.numberOfTuplesToProduce,
                                                                               windowIdentifier,
                                                                               hashJoinProbeHelper.windowSize);
        auto expectedNumberOfTuplesInWindowRight = calculateExpNoTuplesInWindow(hashJoinProbeHelper.numberOfTuplesToProduce,
                                                                                windowIdentifier,
                                                                                hashJoinProbeHelper.windowSize);

        auto existingNumberOfTuplesInWindowLeft =
            hashJoinOpHandler->getNumberOfTuplesInWindow(windowIdentifier, 0, QueryCompilation::JoinBuildSideType::Left);

        auto existingNumberOfTuplesInWindowRight =
            hashJoinOpHandler->getNumberOfTuplesInWindow(windowIdentifier, 0, QueryCompilation::JoinBuildSideType::Right);

        if (existingNumberOfTuplesInWindowLeft != expectedNumberOfTuplesInWindowLeft
            || existingNumberOfTuplesInWindowRight != expectedNumberOfTuplesInWindowRight) {
            x_ERROR(
                "wrong number of inputs are created existingNumberOfTuplesInWindowLeft={} expectedNumberOfTuplesInWindowLeft={} "
                "existingNumberOfTuplesInWindowRight={} expectedNumberOfTuplesInWindowRight={} windowIdentifier={}",
                existingNumberOfTuplesInWindowLeft,
                expectedNumberOfTuplesInWindowLeft,
                existingNumberOfTuplesInWindowRight,
                expectedNumberOfTuplesInWindowRight,
                windowIdentifier);
            EXPECT_TRUE(false);
            EXIT_FAILURE;
        }

        for (auto& leftRecordOuter : leftRecords) {
            for (auto& leftRecordInner : leftRecordOuter) {
                for (auto& rightRecordOuter : rightRecords) {
                    for (auto& rightRecordInner : rightRecordOuter) {
                        auto timestampLeftVal = leftRecordInner.read(hashJoinProbeHelper.timeStampFieldLeft)
                                                    .getValue()
                                                    .staticCast<UInt64>()
                                                    .getValue();
                        auto timestampRightVal = rightRecordInner.read(hashJoinProbeHelper.timeStampFieldRight)
                                                     .getValue()
                                                     .staticCast<UInt64>()
                                                     .getValue();

                        auto windowStart = windowIdentifier - hashJoinProbeHelper.windowSize;
                        auto windowEnd = windowIdentifier;
                        auto leftKey = leftRecordInner.read(hashJoinProbeHelper.joinFieldNameLeft);
                        auto rightKey = rightRecordInner.read(hashJoinProbeHelper.joinFieldNameRight);

                        auto sizeOfWindowStart = sizeof(uint64_t);
                        auto sizeOfWindowEnd = sizeof(uint64_t);

                        DefaultPhysicalTypeFactory physicalDataTypeFactory;
                        auto joinKeySize =
                            physicalDataTypeFactory
                                .getPhysicalType(
                                    hashJoinProbeHelper.leftSchema->get(hashJoinProbeHelper.joinFieldNameLeft)->getDataType())
                                ->size();
                        auto leftTupleSize = hashJoinProbeHelper.leftSchema->getSchemaSizeInBytes();
                        auto rightTupleSize = hashJoinProbeHelper.rightSchema->getSchemaSizeInBytes();

                        if (windowStart <= timestampLeftVal && timestampLeftVal < windowEnd && windowStart <= timestampRightVal
                            && timestampRightVal < windowEnd && leftKey == rightKey) {
                            Record joinedRecord;
                            Nautilus::Value<Any> windowStartVal(windowStart);
                            Nautilus::Value<Any> windowEndVal(windowEnd);
                            joinedRecord.write(joinSchema->get(0)->getName(), windowStartVal);
                            joinedRecord.write(joinSchema->get(1)->getName(), windowEndVal);
                            joinedRecord.write(joinSchema->get(2)->getName(),
                                               leftRecordInner.read(hashJoinProbeHelper.joinFieldNameLeft));

                            // Writing the leftSchema fields
                            for (auto& field : hashJoinProbeHelper.leftSchema->fields) {
                                joinedRecord.write(field->getName(), leftRecordInner.read(field->getName()));
                            }

                            // Writing the rightSchema fields
                            for (auto& field : hashJoinProbeHelper.rightSchema->fields) {
                                joinedRecord.write(field->getName(), rightRecordInner.read(field->getName()));
                            }

                            // Check if this joinedRecord is in the emitted records
                            auto it = std::find(collector->records.begin(), collector->records.end(), joinedRecord);
                            if (it == collector->records.end()) {
                                x_ERROR("Could not find joinedRecord {} in the emitted records!", joinedRecord.toString());
                                return false;
                            }

                            collector->records.erase(it);
                        }
                    }
                }
            }
        }
    }

    return true;
}

class TestRunner : public x::Exceptions::ErrorListener {
  public:
    void onFatalError(int signalNumber, std::string callStack) override {
        std::ostringstream fatalErrorMessage;
        fatalErrorMessage << "onFatalError: signal [" << signalNumber << "] error [" << strerror(errno) << "] callstack "
                          << callStack;

        x_FATAL_ERROR("{}", fatalErrorMessage.str());
        std::cerr << fatalErrorMessage.str() << std::endl;
    }

    void onFatalException(std::shared_ptr<std::exception> exceptionPtr, std::string callStack) override {
        std::ostringstream fatalExceptionMessage;
        fatalExceptionMessage << "onFatalException: exception=[" << exceptionPtr->what() << "] callstack=\n" << callStack;

        x_FATAL_ERROR("{}", fatalExceptionMessage.str());
        std::cerr << fatalExceptionMessage.str() << std::endl;
    }
};

TEST_F(HashJoinOperatorTest, joinBuildTest) {
    // Activating and installing error listener
    auto runner = std::make_shared<TestRunner>();
    x::Exceptions::installGlobalErrorListener(runner);

    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto joinFieldNameLeft = "f2_left";
    const auto timeStampField = "left$timestamp";
    const auto isLeftSide = QueryCompilation::JoinBuildSideType::Left;

    auto handlerIndex = 0;
    auto readTsField = std::make_shared<Expressions::ReadFieldExpression>(timeStampField);

    auto hashJoinBuild = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        isLeftSide,
        joinFieldNameLeft,
        timeStampField,
        leftSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsField));

    HashJoinBuildHelper buildHelper(hashJoinBuild, joinFieldNameLeft, bm, leftSchema, timeStampField, this, isLeftSide);
    ASSERT_TRUE(hashJoinBuildAndCheck(buildHelper));
    // As we are only building here the left side, we do not emit any buffers
    ASSERT_EQ(emittedBuffers.size(), 0);
}

TEST_F(HashJoinOperatorTest, joinBuildTestRight) {
    const auto rightSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                 ->addField("f1_right", BasicType::UINT64)
                                 ->addField("f2_right", BasicType::UINT64)
                                 ->addField("left$timestamp", BasicType::UINT64);

    const auto joinFieldNameRight = "f2_right";
    const auto timeStampField = "left$timestamp";
    const auto isLeftSide = QueryCompilation::JoinBuildSideType::Right;

    auto readTsField = std::make_shared<Expressions::ReadFieldExpression>(timeStampField);
    auto handlerIndex = 0;
    auto hashJoinBuild = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        isLeftSide,
        joinFieldNameRight,
        timeStampField,
        rightSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsField));
    HashJoinBuildHelper buildHelper(hashJoinBuild, joinFieldNameRight, bm, rightSchema, timeStampField, this, isLeftSide);

    ASSERT_TRUE(hashJoinBuildAndCheck(buildHelper));
    // As we are only building here the left side, we do not emit any buffers
    ASSERT_EQ(emittedBuffers.size(), 0);
}

TEST_F(HashJoinOperatorTest, joinBuildTestMultiplePagesPerBucket) {
    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto joinFieldNameLeft = "f2_left";
    const auto timeStampField = "left$timestamp";
    const auto isLeftSide = QueryCompilation::JoinBuildSideType::Left;

    auto handlerIndex = 0;
    auto readTsField = std::make_shared<Expressions::ReadFieldExpression>(timeStampField);

    auto hashJoinBuild = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        isLeftSide,
        joinFieldNameLeft,
        timeStampField,
        leftSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsField));

    HashJoinBuildHelper buildHelper(hashJoinBuild, joinFieldNameLeft, bm, leftSchema, timeStampField, this, isLeftSide);
    buildHelper.pageSize = leftSchema->getSchemaSizeInBytes() * 2;
    buildHelper.numPartitions = 1;

    ASSERT_TRUE(hashJoinBuildAndCheck(buildHelper));
    // As we are only building here the left side, we do not emit any buffers
    ASSERT_EQ(emittedBuffers.size(), 0);
}

TEST_F(HashJoinOperatorTest, joinBuildTestMultipleWindows) {
    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto joinFieldNameLeft = "f2_left";
    const auto timeStampField = "left$timestamp";
    const auto handlerIndex = 0;
    const auto isLeftSide = QueryCompilation::JoinBuildSideType::Left;

    auto readTsField = std::make_shared<Expressions::ReadFieldExpression>(timeStampField);

    auto hashJoinBuild = std::make_shared<Operators::StreamHashJoinBuild>(
        handlerIndex,
        isLeftSide,
        joinFieldNameLeft,
        timeStampField,
        leftSchema,
        std::make_unique<Runtime::Execution::Operators::EventTimeFunction>(readTsField));

    HashJoinBuildHelper buildHelper(hashJoinBuild, joinFieldNameLeft, bm, leftSchema, timeStampField, this, isLeftSide);
    buildHelper.pageSize = leftSchema->getSchemaSizeInBytes() * 2, buildHelper.numPartitions = 1;
    buildHelper.windowSize = 5;

    ASSERT_TRUE(hashJoinBuildAndCheck(buildHelper));
    // As we are only building here the left side, we do not emit any buffers
    ASSERT_EQ(emittedBuffers.size(), 0);
}

TEST_F(HashJoinOperatorTest, joinProbeTest) {
    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto rightSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                 ->addField("f1_right", BasicType::UINT64)
                                 ->addField("f2_right", BasicType::UINT64)
                                 ->addField("right$timestamp", BasicType::UINT64);

    ASSERT_EQ(leftSchema->getSchemaSizeInBytes(), rightSchema->getSchemaSizeInBytes());

    HashJoinProbeHelper
        hashJoinProbeHelper("f2_left", "f2_right", bm, leftSchema, rightSchema, "left$timestamp", "right$timestamp", this);
    hashJoinProbeHelper.pageSize = 2 * leftSchema->getSchemaSizeInBytes();
    hashJoinProbeHelper.numPartitions = 2;
    hashJoinProbeHelper.windowSize = 20;

    ASSERT_TRUE(hashJoinProbeAndCheck(hashJoinProbeHelper));
}

TEST_F(HashJoinOperatorTest, joinProbeTestMultipleBuckets) {
    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto rightSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                 ->addField("f1_right", BasicType::UINT64)
                                 ->addField("f2_right", BasicType::UINT64)
                                 ->addField("right$timestamp", BasicType::UINT64);

    ASSERT_EQ(leftSchema->getSchemaSizeInBytes(), rightSchema->getSchemaSizeInBytes());

    HashJoinProbeHelper
        hashJoinProbeHelper("f2_left", "f2_right", bm, leftSchema, rightSchema, "left$timestamp", "right$timestamp", this);
    hashJoinProbeHelper.windowSize = 10;

    ASSERT_TRUE(hashJoinProbeAndCheck(hashJoinProbeHelper));
}

TEST_F(HashJoinOperatorTest, joinProbeTestMultipleWindows) {

    const auto leftSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                ->addField("f1_left", BasicType::UINT64)
                                ->addField("f2_left", BasicType::UINT64)
                                ->addField("left$timestamp", BasicType::UINT64);

    const auto rightSchema = Schema::create(Schema::MemoryLayoutType::ROW_LAYOUT)
                                 ->addField("f1_right", BasicType::UINT64)
                                 ->addField("f2_right", BasicType::UINT64)
                                 ->addField("right$timestamp", BasicType::UINT64);
    ASSERT_EQ(leftSchema->getSchemaSizeInBytes(), rightSchema->getSchemaSizeInBytes());

    HashJoinProbeHelper
        hashJoinProbeHelper("f2_left", "f2_right", bm, leftSchema, rightSchema, "left$timestamp", "right$timestamp", this);
    hashJoinProbeHelper.numPartitions = 1;
    hashJoinProbeHelper.windowSize = 10;

    ASSERT_TRUE(hashJoinProbeAndCheck(hashJoinProbeHelper));
}

}// namespace x::Runtime::Execution