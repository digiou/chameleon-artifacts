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
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/MemoryLayout/ColumnLayout.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/MemoryLayout/RowLayout.hpp>
#include <Runtime/QueryManager.hpp>
#include <Sensors/Values/SingleSensor.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Sources/DataSource.hpp>
#include <Sources/ZmqSource.hpp>
#include <Util/Common.hpp>
#include <Util/Core.hpp>
#include <Util/KalmanFilter.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadNaming.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <thread>

#ifdef x_USE_ONE_QUEUE_PER_NUMA_NODE
#if defined(__linux__)
#include <numa.h>
#include <numaif.h>
#endif
#endif
#include <utility>
using namespace std::string_literals;
namespace x {

std::vector<Runtime::Execution::SuccessorExecutablePipeline> DataSource::getExecutableSuccessors() {
    return executableSuccessors;
}

void DataSource::addExecutableSuccessors(std::vector<Runtime::Execution::SuccessorExecutablePipeline> newPipelix) {
    successorModifyMutex.lock();
    for (auto& pipe : newPipelix) {
        executableSuccessors.push_back(pipe);
    }
}

DataSource::DataSource(SchemaPtr pSchema,
                       Runtime::BufferManagerPtr bufferManager,
                       Runtime::QueryManagerPtr queryManager,
                       OperatorId operatorId,
                       OriginId originId,
                       size_t numSourceLocalBuffers,
                       GatheringMode gatheringMode,
                       const std::string& physicalSourceName,
                       std::vector<Runtime::Execution::SuccessorExecutablePipeline> executableSuccessors,
                       uint64_t sourceAffinity,
                       uint64_t taskQueueId)
    : Runtime::Reconfigurable(), DataEmitter(), queryManager(std::move(queryManager)),
      localBufferManager(std::move(bufferManager)), executableSuccessors(std::move(executableSuccessors)), operatorId(operatorId),
      originId(originId), schema(std::move(pSchema)), numSourceLocalBuffers(numSourceLocalBuffers), gatheringMode(gatheringMode),
      sourceAffinity(sourceAffinity), taskQueueId(taskQueueId), physicalSourceName(physicalSourceName), kFilter(std::make_unique<KalmanFilter>()),
      lastValuesBuf(lastValuesSize), lastIntervalBuf(lastValuesSize) {
    this->kFilter->init();
    x_DEBUG("DataSource  {} : Init Data Source with schema  {}", operatorId, schema->toString());
    x_ASSERT(this->localBufferManager, "Invalid buffer manager");
    x_ASSERT(this->queryManager, "Invalid query manager");
    // TODO #4094: enable this exception -- currently many UTs are designed to assume empty executableSuccessors
    //    if (this->executableSuccessors.empty()) {
    //        throw Exceptions::RuntimeException("empty executable successors");
    //    }
    if (schema->getLayoutType() == Schema::MemoryLayoutType::ROW_LAYOUT) {
        memoryLayout = Runtime::MemoryLayouts::RowLayout::create(schema, localBufferManager->getBufferSize());
    } else if (schema->getLayoutType() == Schema::MemoryLayoutType::COLUMNAR_LAYOUT) {
        memoryLayout = Runtime::MemoryLayouts::ColumnLayout::create(schema, localBufferManager->getBufferSize());
    }
}

void DataSource::emitWorkFromSource(Runtime::TupleBuffer& buffer) {
    // set the origin id for this source
    buffer.setOriginId(originId);
    // set the creation timestamp
    buffer.setCreationTimestampInMS(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
            .count());
    // Set the sequence number of this buffer.
    // A data source generates a monotonic increasing sequence number
    maxSequenceNumber++;
    buffer.setSequenceNumber(maxSequenceNumber);
    emitWork(buffer);
}

void DataSource::emitWork(Runtime::TupleBuffer& buffer) {
    uint64_t queueId = 0;
    for (const auto& successor : executableSuccessors) {
        //find the queue to which this sources pushes
        if (!sourceSharing) {
            queryManager->addWorkForNextPipeline(buffer, successor, taskQueueId);
        } else {
            x_DEBUG("push task for queueid= {} successor= ", queueId /*, &successor*/);
            queryManager->addWorkForNextPipeline(buffer, successor, queueId);
        }
    }
}

OperatorId DataSource::getOperatorId() const { return operatorId; }

void DataSource::setOperatorId(OperatorId operatorId) { this->operatorId = operatorId; }

SchemaPtr DataSource::getSchema() const { return schema; }

DataSource::~DataSource() x_NOEXCEPT(false) {
    x_ASSERT(running == false, "Data source destroyed but thread still running... stop() was not called");
    x_DEBUG("DataSource {}: Destroy Data Source.", operatorId);
    executableSuccessors.clear();
}

bool DataSource::start() {
    x_DEBUG("DataSource  {} : start source", operatorId);
    std::promise<bool> prom;
    std::unique_lock lock(startStopMutex);
    bool expected = false;
    std::shared_ptr<std::thread> thread{nullptr};
    if (!running.compare_exchange_strong(expected, true)) {
        x_WARNING("DataSource {}: is already running", operatorId);
        return false;
    } else {
        type = getType();
        x_DEBUG("DataSource {}: Spawn thread", operatorId);
        auto expected = false;
        if (wasStarted.compare_exchange_strong(expected, true)) {
            thread = std::make_shared<std::thread>([this, &prom]() {
            // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
            // only CPU i as set.
#ifdef __linux__
                if (sourceAffinity != std::numeric_limits<uint64_t>::max()) {
                    x_ASSERT(sourceAffinity < std::thread::hardware_concurrency(),
                               "pinning position is out of cpu range maxPosition=" << sourceAffinity);
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    CPU_SET(sourceAffinity, &cpuset);
                    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                    if (rc != 0) {
                        x_THROW_RUNTIME_ERROR("Cannot set thread affinity on source thread " + std::to_string(operatorId));
                    }
                } else {
                    x_WARNING("Use default affinity for source");
                }
#endif
                prom.set_value(true);
                runningRoutine();
                x_DEBUG("DataSource {}: runningRoutine is finished", operatorId);
            });
        }
    }
    if (thread) {
        thread->detach();
    }
    return prom.get_future().get();
}

bool DataSource::fail() {
    bool isStopped = stop(Runtime::QueryTerminationType::Failure);// this will block until the thread is stopped
    x_DEBUG("Source {} stop executed= {}", operatorId, (isStopped ? "stopped" : "cannot stop"));
    {
        // it may happen that the source failed prior of sending its eos
        std::unique_lock lock(startStopMutex);// do not call stop if holding this mutex
        auto self = shared_from_base<DataSource>();
        x_DEBUG("Source {} has already injected failure? {}", operatorId, (endOfStreamSent ? "EoS sent" : "cannot send EoS"));
        if (!this->endOfStreamSent) {
            endOfStreamSent = queryManager->addEndOfStream(self, Runtime::QueryTerminationType::Failure);
            queryManager->notifySourceCompletion(self, Runtime::QueryTerminationType::Failure);
            x_DEBUG("Source {} injecting failure  {}", operatorId, (endOfStreamSent ? "EoS sent" : "cannot send EoS"));
        }
        return isStopped && endOfStreamSent;
    }
    return false;
}

namespace detail {
template<typename R, typename P>
bool waitForFuture(std::future<bool>&& future, std::chrono::duration<R, P>&& deadline) {
    auto terminationStatus = future.wait_for(deadline);
    switch (terminationStatus) {
        case std::future_status::ready: {
            return future.get();
        }
        default: {
            return false;
        }
    }
}
}// namespace detail

bool DataSource::stop(Runtime::QueryTerminationType graceful) {
    using namespace std::chrono_literals;
    // Do not call stop from the runningRoutine!
    {
        std::unique_lock lock(startStopMutex);// this mutex guards the thread variable
        wasGracefullyStopped = graceful;
    }

    refCounter++;
    if (refCounter != numberOfConsumerQueries) {
        return true;
    }

    x_DEBUG("DataSource {}: Stop called and source is {}", operatorId, (running ? "running" : "not running"));
    bool expected = true;

    // TODO add wakeUp call if source is blocking on something, e.g., tcp socket
    // TODO in general this highlights how our source model has some issues

    // TODO this is also an issue in the current development of the StaticDataSource. If it is still running here, we give up to early and never join the thread.

    try {
        if (!running.compare_exchange_strong(expected, false)) {
            x_DEBUG("DataSource {} was not running, retrieving future now...", operatorId);
            auto expected = false;
            if (wasStarted && futureRetrieved.compare_exchange_strong(expected, true)) {
                x_ASSERT2_FMT(detail::waitForFuture(completedPromise.get_future(), 60s),
                                "Cannot complete future to stop source " << operatorId);
            }
            x_DEBUG("DataSource {} was not running, future retrieved", operatorId);
            return true;// it's ok to return true because the source is stopped
        } else {
            x_DEBUG("DataSource {} was running, retrieving future now...", operatorId);
            auto expected = false;
            x_ASSERT2_FMT(wasStarted && futureRetrieved.compare_exchange_strong(expected, true)
                                && detail::waitForFuture(completedPromise.get_future(), 10min),
                            "Cannot complete future to stop source " << operatorId);
            x_WARNING("Stopped Source {} = {}", operatorId, wasGracefullyStopped);
            return true;
        }
    } catch (...) {
        auto expPtr = std::current_exception();
        wasGracefullyStopped = Runtime::QueryTerminationType::Failure;
        try {
            if (expPtr) {
                std::rethrow_exception(expPtr);
            }
        } catch (std::exception const& e) {// it would not work if you pass by value
            // i leave the following lix just as a reminder:
            // here we do not need to call notifySourceFailure because it is done from the main thread
            // the only reason to call notifySourceFailure is when the main thread was not stated
            if (!wasStarted) {
                queryManager->notifySourceFailure(shared_from_base<DataSource>(), std::string(e.what()));
            }
            return true;
        }
    }

    return false;
}

void DataSource::setGatheringInterval(std::chrono::milliseconds interval) { this->gatheringInterval = interval; }

void DataSource::open() { bufferManager = localBufferManager->createFixedSizeBufferPool(numSourceLocalBuffers); }

void DataSource::close() {
    Runtime::QueryTerminationType queryTerminationType;
    {
        std::unique_lock lock(startStopMutex);
        queryTerminationType = this->wasGracefullyStopped;
    }
    if (queryTerminationType != Runtime::QueryTerminationType::Graceful
        || queryManager->canTriggerEndOfStream(shared_from_base<DataSource>(), queryTerminationType)) {
        // inject reconfiguration task containing end of stream
        std::unique_lock lock(startStopMutex);
        x_ASSERT2_FMT(!endOfStreamSent, "Eos was already sent for source " << toString());
        x_DEBUG("DataSource {} : Data Source add end of stream. Gracefully={}", operatorId, queryTerminationType);
        endOfStreamSent = queryManager->addEndOfStream(shared_from_base<DataSource>(), queryTerminationType);
        x_ASSERT2_FMT(endOfStreamSent, "Cannot send eos for source " << toString());
        bufferManager->destroy();
        queryManager->notifySourceCompletion(shared_from_base<DataSource>(), queryTerminationType);
    }
}

void DataSource::runningRoutine() {
    try {
        if (gatheringMode == GatheringMode::INTERVAL_MODE) {
            runningRoutineWithGatheringInterval();
        } else if (gatheringMode == GatheringMode::INGESTION_RATE_MODE) {
            runningRoutineWithIngestionRate();
        } else if (gatheringMode == GatheringMode::ADAPTIVE_MODE) {
            runningRoutineAdaptiveGatheringInterval();
        } else if (gatheringMode == GatheringMode::ADAPTIVE_MODE_OVERSAMPLER) {
            runningRoutineAdaptiveGatheringIntervalOversampler();
        }
        completedPromise.set_value(true);
    } catch (std::exception const& exception) {
        queryManager->notifySourceFailure(shared_from_base<DataSource>(), exception.what());
        completedPromise.set_exception(std::make_exception_ptr(exception));
    } catch (...) {
        try {
            auto expPtr = std::current_exception();
            if (expPtr) {
                completedPromise.set_exception(expPtr);
                std::rethrow_exception(expPtr);
            }
        } catch (std::exception const& exception) {
            queryManager->notifySourceFailure(shared_from_base<DataSource>(), exception.what());
        }
    }
    x_DEBUG("DataSource {} end runningRoutine", operatorId);
}

void DataSource::runningRoutineWithIngestionRate() {
    x_ASSERT(this->operatorId != 0, "The id of the source is not set properly");
    x_ASSERT(gatheringIngestionRate >= 10, "As we generate on 100 ms base we need at least an ingestion rate of 10");
    std::string thName = "DataSrc-" + std::to_string(operatorId);
    setThreadName(thName.c_str());

    x_DEBUG("DataSource {} Running Data Source of type={} ingestion rate={}",
              operatorId,
              magic_enum::enum_name(getType()),
              gatheringIngestionRate);
    if (numberOfBuffersToProduce == 0) {
        x_DEBUG(
            "DataSource: the user does not specify the number of buffers to produce therefore we will produce buffers until "
            "the source is empty");
    } else {
        x_DEBUG("DataSource: the user specify to produce {} buffers", numberOfBuffersToProduce);
    }
    open();

    uint64_t nextPeriodStartTime = 0;
    uint64_t curPeriod = 0;
    uint64_t processedOverallBufferCnt = 0;
    uint64_t buffersToProducePer100Ms = gatheringIngestionRate / 10;
    while (running) {
        //create as many tuples as requested and then sleep
        auto startPeriod =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        uint64_t buffersProcessedCnt = 0;

        //produce buffers until limit for this second or for all perionds is reached or source is topped
        while (buffersProcessedCnt < buffersToProducePer100Ms && running
               && (processedOverallBufferCnt < numberOfBuffersToProduce || numberOfBuffersToProduce == 0)) {
            auto optBuf = receiveData();

            if (optBuf.has_value()) {
                // here we got a valid buffer
                x_TRACE("DataSource: add task for buffer");
                auto& buf = optBuf.value();
                emitWorkFromSource(buf);

                buffersProcessedCnt++;
                processedOverallBufferCnt++;
            } else {
                x_ERROR("DataSource: Buffer is invalid");
                running = false;
            }
            x_TRACE("DataSource: buffersProcessedCnt={} buffersPerSecond={}", buffersProcessedCnt, gatheringIngestionRate);
        }

        uint64_t endPeriod =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        //next point in time when to start producing again
        nextPeriodStartTime = uint64_t(startPeriod + (100));
        x_TRACE("DataSource: startTimeSendBuffers={} endTimeSendBuffers={} nextPeriodStartTime={}",
                  startPeriod,
                  endPeriod,
                  nextPeriodStartTime);

        //If this happens then the second was not enough to create so many tuples and the ingestion rate should be decreased
        if (nextPeriodStartTime < endPeriod) {
            x_ERROR(
                "Creating buffer(s) for DataSource took longer than periodLength. nextPeriodStartTime={} endTimeSendBuffers={}",
                nextPeriodStartTime,
                endPeriod);
        }

        uint64_t sleepCnt = 0;
        uint64_t curTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        //wait until the next period starts
        while (curTime < nextPeriodStartTime) {
            curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                          .count();
        }
        x_DEBUG("DataSource: Done with period {} and overall buffers={} sleepCnt={} startPeriod={} endPeriod={} "
                  "nextPeriodStartTime={} curTime={}",
                  curPeriod++,
                  processedOverallBufferCnt,
                  sleepCnt,
                  startPeriod,
                  endPeriod,
                  nextPeriodStartTime,
                  curTime);
    }//end of while
    x_DEBUG("DataSource {} call close", operatorId);
    close();
    x_DEBUG("DataSource {} end running", operatorId);
}

void DataSource::runningRoutineWithGatheringInterval() {
    x_ASSERT(this->operatorId != 0, "The id of the source is not set properly");
    std::string thName = "DataSrc-" + std::to_string(operatorId);
    setThreadName(thName.c_str());

    x_DEBUG("DataSource {}: Running Data Source of type={} interval={}",
              operatorId,
              magic_enum::enum_name(getType()),
              gatheringInterval.count());
    if (numberOfBuffersToProduce == 0) {
        x_DEBUG("DataSource: the user does not specify the number of buffers to produce therefore we will produce buffer until "
                  "the source is empty");
    } else {
        x_DEBUG("DataSource: the user specify to produce {} buffers", numberOfBuffersToProduce);
    }
    open();
    uint64_t numberOfBuffersProduced = 0;
    while (running) {
        //check if already produced enough buffer
        if (numberOfBuffersToProduce == 0 || numberOfBuffersProduced < numberOfBuffersToProduce) {
            auto optBuf = receiveData();// note that receiveData might block
            if (!running) {             // necessary if source stops while receiveData is called due to stricter shutdown logic
                break;
            }
            //this checks we received a valid output buffer
            if (optBuf.has_value()) {
                auto& buf = optBuf.value();
                x_TRACE("DataSource produced buffer {} type= {} string={}: Received Data: {} tuples iteration= {} "
                          "operatorId={} orgID={}",
                          operatorId,
                          magic_enum::enum_name(getType()),
                          toString(),
                          buf.getNumberOfTuples(),
                          numberOfBuffersProduced,
                          this->operatorId,
                          this->operatorId);

                if (Logger::getInstance()->getCurrentLogLevel() == LogLevel::LOG_TRACE) {
                    auto layout = Runtime::MemoryLayouts::RowLayout::create(schema, buf.getBufferSize());
                    auto buffer = Runtime::MemoryLayouts::DynamicTupleBuffer(layout, buf);
                    x_TRACE("DataSource produced buffer content={}", buffer.toString(schema));
                }

                emitWorkFromSource(buf);
                ++numberOfBuffersProduced;
            } else {
                x_DEBUG("DataSource {}: stopping cause of invalid buffer", operatorId);
                running = false;
                x_DEBUG("DataSource {}: Thread going to terminating with graceful exit.", operatorId);
            }
        } else {
            x_DEBUG("DataSource {}: Receiving thread terminated ... stopping because cnt={} smaller than "
                      "numBuffersToProcess={} now return",
                      operatorId,
                      numberOfBuffersProduced,
                      numberOfBuffersToProduce);
            running = false;
        }
        x_TRACE("DataSource {} : Data Source finished processing iteration {}", operatorId, numberOfBuffersProduced);

        // this checks if the interval is zero or a ZMQ_Source, we don't create a watermark-only buffer
        if (getType() != SourceType::ZMQ_SOURCE && gatheringInterval.count() > 0) {
//            std::this_thread::sleep_for(gatheringInterval);
        }
    }
    x_DEBUG("DataSource {} call close", operatorId);
    close();

    x_DEBUG("DataSource {} end running", operatorId);
}

void DataSource::runningRoutineAdaptiveGatheringInterval() {
    x_ASSERT(this->operatorId != 0, "The id of the source is not set properly");
    std::string thName = "DataSrc-" + std::to_string(operatorId);
    setThreadName(thName.c_str());

    x_TRACE("DataSource {}: Running Data Source of type={} interval={}",
              operatorId,
              magic_enum::enum_name(getType()),
              gatheringInterval.count());

    if (numberOfBuffersToProduce == 0) {
        x_TRACE("DataSource: the user does not specify the number of buffers to produce therefore we will produce buffer until "
                  "the source is empty");
    } else {
        x_TRACE("DataSource: the user specify to produce {} buffers", numberOfBuffersToProduce);
    }

    this->kFilter->setGatheringInterval(this->gatheringInterval);
    this->kFilter->setGatheringIntervalRange(std::chrono::milliseconds{8000});

    open();
    uint64_t numberOfBuffersProduced = 0;
    while (running) {
        //check if already produced enough buffer
        if (numberOfBuffersToProduce == 0 || numberOfBuffersProduced < numberOfBuffersToProduce) {
            auto optBuf = receiveData();// note that receiveData might block
            if (!running) {             // necessary if source stops while receiveData is called due to stricter shutdown logic
                break;
            }

            //this checks we received a valid output buffer
            if (optBuf.has_value()) {
                auto& buf = optBuf.value();
                auto numOfTuples = buf.getNumberOfTuples();
                auto records = buf.getBuffer<Sensors::SingleSensor>();
                double currentIntervalInSeconds = this->gatheringInterval.count() / 1000.;
                for (uint64_t i = 0; i < numOfTuples; ++i) {
                    this->lastValuesBuf.emplace(records[i].value);
                    this->lastIntervalBuf.emplace(currentIntervalInSeconds);
                }

                // use a vector, find mean interval at the same time
                double totalIntervalInseconds = 0;
                for (uint64_t idx=0; idx < this->lastValuesBuf.size(); ++idx) {
                    totalIntervalInseconds += this->lastIntervalBuf.at(idx);
                }
                totalIntervalInseconds /= this->lastValuesBuf.size();
                double skewedIntervalInseconds = (totalIntervalInseconds + currentIntervalInSeconds) / 2.;

                std::tuple<bool, double> res = x::Util::computeNyquistAndEnergy(this->lastValuesBuf.toVector(), skewedIntervalInseconds);
                if (std::get<0>(res)) { // nyq rate is smaller than current skewed median interval
                    this->kFilter->setSlowestInterval(std::move(std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(std::get<1>(res))))));
                }

                this->kFilter->updateFromTupleBuffer(buf);
                this->gatheringInterval = this->kFilter->getNewGatheringInterval();

                 emitWorkFromSource(buf);
                ++numberOfBuffersProduced;
            } else {
                x_ERROR("DataSource {}: stopping cause of invalid buffer", operatorId);
                running = false;
            }
        } else {
            running = false;
        }
        // this checks if the interval is zero or a ZMQ_Source, we don't create a watermark-only buffer
        if (getType() != SourceType::ZMQ_SOURCE && gatheringInterval.count() > 0) {
            x_TRACE("DataSource {} sleeping on interval {}", operatorId, gatheringInterval.count());
//            std::this_thread::sleep_for(gatheringInterval);
        }
    }

    close();
}

void DataSource::runningRoutineAdaptiveGatheringIntervalOversampler() {
    x_ASSERT(this->operatorId != 0, "The id of the source is not set properly");
    std::string thName = "DataSrc-" + std::to_string(operatorId);
    setThreadName(thName.c_str());

    x_TRACE("DataSource {}: Running Data Source of type={} interval={}",
              operatorId,
              magic_enum::enum_name(getType()),
              gatheringInterval.count());

    if (numberOfBuffersToProduce == 0) {
        x_TRACE("DataSource: the user does not specify the number of buffers to produce therefore we will produce buffer until "
                  "the source is empty");
    } else {
        x_TRACE("DataSource: the user specify to produce {} buffers", numberOfBuffersToProduce);
    }

    this->kFilter->setGatheringInterval(this->gatheringInterval);
    this->kFilter->setGatheringIntervalRange(std::chrono::milliseconds{8000});

    open();
    uint64_t numberOfBuffersProduced = 0;
    while (running) {
        //check if already produced enough buffer
        if (numberOfBuffersToProduce == 0 || numberOfBuffersProduced < numberOfBuffersToProduce) {
            auto optBuf = receiveData();// note that receiveData might block
            if (!running) {             // necessary if source stops while receiveData is called due to stricter shutdown logic
                break;
            }

            //this checks we received a valid output buffer
            if (optBuf.has_value()) {
                auto& buf = optBuf.value();
                auto numOfTuples = buf.getNumberOfTuples();
                auto records = buf.getBuffer<Sensors::SingleSensor>();
                double currentIntervalInSeconds = this->gatheringInterval.count() / 1000.;
                for (uint64_t i = 0; i < numOfTuples; ++i) {
                    this->lastValuesBuf.emplace(records[i].value);
                    this->lastIntervalBuf.emplace(currentIntervalInSeconds);
                }

                // use a vector, find mean interval at the same time
                double totalIntervalInseconds = 0;
                for (uint64_t idx=0; idx < this->lastValuesBuf.size(); ++idx) {
                    totalIntervalInseconds += this->lastIntervalBuf.at(idx);
                }
                totalIntervalInseconds /= this->lastValuesBuf.size();
                double skewedIntervalInseconds = (totalIntervalInseconds + currentIntervalInSeconds) / 2.;

                std::tuple<bool, double> res = x::Util::computeNyquistAndEnergy(this->lastValuesBuf.toVector(), skewedIntervalInseconds);
                if (std::get<0>(res)) { // nyq rate is smaller than current skewed median interval
                    this->kFilter->setSlowestInterval(std::move(std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(std::get<1>(res))))));
                }

                this->kFilter->getNewGatheringInterval();

                emitWorkFromSource(buf);
                ++numberOfBuffersProduced;
            } else {
                x_ERROR("DataSource {}: stopping cause of invalid buffer", operatorId);
                running = false;
            }
        } else {
            running = false;
        }
        // this checks if the interval is zero or a ZMQ_Source, we don't create a watermark-only buffer
        if (getType() != SourceType::ZMQ_SOURCE && gatheringInterval.count() > 0) {
            x_TRACE("DataSource {} sleeping on interval {}", operatorId, gatheringInterval.count());
//            std::this_thread::sleep_for(gatheringInterval);
        }
    }

    close();
}

bool DataSource::injectEpochBarrier(uint64_t epochBarrier, uint64_t queryId) {
    x_DEBUG("DataSource::injectEpochBarrier received timestamp  {} with queryId  {}", epochBarrier, queryId);
    return queryManager->addEpochPropagation(shared_from_base<DataSource>(), queryId, epochBarrier);
}

// debugging
uint64_t DataSource::getNumberOfGeneratedTuples() const { return generatedTuples; };
uint64_t DataSource::getNumberOfGeneratedBuffers() const { return generatedBuffers; };

std::string DataSource::getSourceSchemaAsString() { return schema->toString(); }

uint64_t DataSource::getNumBuffersToProcess() const { return numberOfBuffersToProduce; }

std::chrono::milliseconds DataSource::getGatheringInterval() const { return gatheringInterval; }
uint64_t DataSource::getGatheringIntervalCount() const { return gatheringInterval.count(); }
std::vector<Schema::MemoryLayoutType> DataSource::getSupportedLayouts() { return {Schema::MemoryLayoutType::ROW_LAYOUT}; }

bool DataSource::checkSupportedLayoutTypes(SchemaPtr& schema) {
    auto supportedLayouts = getSupportedLayouts();
    return std::find(supportedLayouts.begin(), supportedLayouts.end(), schema->getLayoutType()) != supportedLayouts.end();
}

Runtime::MemoryLayouts::DynamicTupleBuffer DataSource::allocateBuffer() {
    auto buffer = bufferManager->getBufferBlocking();
    return Runtime::MemoryLayouts::DynamicTupleBuffer(memoryLayout, buffer);
}

void DataSource::onEvent(Runtime::BaseEvent& event) {
    x_DEBUG("DataSource::onEvent(event) called. operatorId: {}", this->operatorId);
    // no behaviour needed, call onEvent of direct ancestor
    DataEmitter::onEvent(event);
}

void DataSource::onEvent(Runtime::BaseEvent& event, Runtime::WorkerContextRef) {
    x_DEBUG("DataSource::onEvent(event, wrkContext) called. operatorId:  {}", this->operatorId);
    onEvent(event);
}

}// namespace x