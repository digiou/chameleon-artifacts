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

#ifndef x_CORE_INCLUDE_SINKS_MEDIUMS_SINKMEDIUM_HPP_
#define x_CORE_INCLUDE_SINKS_MEDIUMS_SINKMEDIUM_HPP_

#include <Runtime/Reconfigurable.hpp>
#include <Sinks/Formats/SinkFormat.hpp>
#include <Util/FaultToleranceType.hpp>
#include <Windowing/Watermark/MultiOriginWatermarkProcessor.hpp>
#include <mutex>

namespace x {

enum class SinkMediumTypes : uint8_t {
    ZMQ_SINK,
    PRINT_SINK,
    KAFKA_SINK,
    FILE_SINK,
    NETWORK_SINK,
    OPC_SINK,
    MQTT_SINK,
    NULL_SINK,
    MATERIALIZED_VIEW_SINK,
    MONITORING_SINK
};

/**
 * @brief Base class for all data sinks in x
 * @note this code is not thread safe
 */
class SinkMedium : public Runtime::Reconfigurable {

  public:
    /**
     * @brief public constructor for data sink
     */
    explicit SinkMedium(SinkFormatPtr sinkFormat,
                        Runtime::NodeEnginePtr nodeEngine,
                        uint32_t numOfProducers,
                        QueryId queryId,
                        QuerySubPlanId querySubPlanId);

    /**
     * @brief public constructor for data sink
     */
    explicit SinkMedium(SinkFormatPtr sinkFormat,
                        Runtime::NodeEnginePtr nodeEngine,
                        uint32_t numOfProducers,
                        QueryId queryId,
                        QuerySubPlanId querySubPlanId,
                        FaultToleranceType faultToleranceType,
                        uint64_t numberOfOrigins,
                        Windowing::MultiOriginWatermarkProcessorPtr watermarkProcessor);

    /**
     * @brief virtual method to setup sink
     * @Note this method will be overwritten by derived classes
     */
    virtual void setup() = 0;

    /**
     * @brief virtual method to shutdown sink
     * @Note this method will be overwritten by derived classes
     */
    virtual void shutdown() = 0;

    /**
     * @brief method to write a TupleBuffer
     * @param a tuple buffers pointer
     * @return bool indicating if the write was complete
     */
    //Todo: In the scope of #4040 we decide whether writeData() should return an ExecutionResult
    virtual bool writeData(Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContext& workerContext) = 0;

    /**
     * @brief get the id of the owning plan
     * @return queryId
     */
    QueryId getQueryId() const;

    /**
     * @brief get the suzbplan id of the owning plan
     * @return QuerySubPlanId
     */
    QuerySubPlanId getParentPlanId() const;

    /**
     * @brief debug function for testing to get number of written buffers
     * @return number of sent buffer
     */
    uint64_t getNumberOfWrittenOutBuffers();

    /**
     * @brief debug function for testing to get number of written tuples
     * @return number of sent buffer
     */
    uint64_t getNumberOfWrittenOutTuples();

    /**
     * @brief virtual function to get a string describing the particular sink
     * @Note this function is overwritten by the particular data sink
     * @return string with name and additional information about the sink
     */
    virtual std::string toString() const = 0;

    /**
   * @brief method to return the current schema of the sink
   * @return schema description of the sink
   */
    SchemaPtr getSchemaPtr() const;

    /**
     * @brief method to get the format as string
     * @return format as string
     */
    std::string getSinkFormat();

    /**
     * @brief method passes current safe to trim timestamp to coordinator via RPC
     * @param epochBarrier max epoch timestamp
     * @return success
     */
    bool notifyEpochTermination(uint64_t epochBarrier) const;

    /**
      * @brief method to return the type of medium
      * @return type of medium
      */
    virtual SinkMediumTypes getSinkMediumType() = 0;

    /**
     * @brief
     * @param message
     * @param context
     */
    void reconfigure(Runtime::ReconfigurationMessage& message, Runtime::WorkerContext& context) override;

    /**
     * @brief
     * @param message
     */
    void postReconfigurationCallback(Runtime::ReconfigurationMessage& message) override;

    /**
     * @brief
     * @return
     */
    OperatorId getOperatorId() const;

    /**
     * @brief returns current smallest timestamp stored in multi origin watermark processor
     * @return epoch barrier
     */
    uint64_t getCurrentEpochBarrier();

    /**
     * @brief update watermark and propagate timestamp
     * @param inputBuffer
     */
    void updateWatermark(Runtime::TupleBuffer& inputBuffer);

  protected:
    SinkFormatPtr sinkFormat;
    uint32_t bufferCount;
    uint32_t buffersPerEpoch;
    bool schemaWritten;

    Runtime::NodeEnginePtr nodeEngine;
    /// termination machinery
    std::atomic<uint32_t> activeProducers;
    QueryId queryId;
    QuerySubPlanId querySubPlanId;
    FaultToleranceType faultToleranceType;
    uint64_t numberOfOrigins;
    Windowing::MultiOriginWatermarkProcessorPtr watermarkProcessor;
    std::function<void(Runtime::TupleBuffer&)> updateWatermarkCallback;

    uint64_t sentBuffer{0};
    uint64_t sentTuples{0};
    std::recursive_mutex writeMutex;
};

using DataSinkPtr = std::shared_ptr<SinkMedium>;

}// namespace x

#endif// x_CORE_INCLUDE_SINKS_MEDIUMS_SINKMEDIUM_HPP_
