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

#include <Runtime/QueryManager.hpp>
#include <Sinks/Mediums/ZmqSink.hpp>
#include <Util/Logger/Logger.hpp>
#include <cstdint>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <zmq.hpp>

namespace x {

SinkMediumTypes ZmqSink::getSinkMediumType() { return SinkMediumTypes::ZMQ_SINK; }

ZmqSink::ZmqSink(SinkFormatPtr format,
                 Runtime::NodeEnginePtr nodeEngine,
                 uint32_t numOfProducers,
                 const std::string& host,
                 uint16_t port,
                 bool internal,
                 QueryId queryId,
                 QuerySubPlanId querySubPlanId,
                 FaultToleranceType faultToleranceType,
                 uint64_t numberOfOrigins)
    : SinkMedium(std::move(format),
                 std::move(nodeEngine),
                 numOfProducers,
                 queryId,
                 querySubPlanId,
                 faultToleranceType,
                 numberOfOrigins,
                 std::make_unique<Windowing::MultiOriginWatermarkProcessor>(numberOfOrigins)),
      host(host.substr(0, host.find(':'))), port(port), internal(internal), context(zmq::context_t(1)),
      socket(zmq::socket_t(context, ZMQ_PUSH)) {
    x_DEBUG("ZmqSink: Init ZMQ Sink to {}:{}", host, port);
}

void ZmqSink::setup() { connect(); };
void ZmqSink::shutdown(){};

ZmqSink::~ZmqSink() {
    x_DEBUG("ZmqSink::~ZmqSink: destructor called");
    bool success = disconnect();
    if (success) {
        x_DEBUG("ZmqSink: Destroy ZMQ Sink");
    } else {
        /// XXX:
        x_ERROR("ZmqSink: Destroy ZMQ Sink failed cause it could not be disconnected");
        x_ASSERT2_FMT(false, "ZMQ Sink destruction failed");
    }
    x_DEBUG("ZmqSink: Destroy ZMQ Sink");
}

bool ZmqSink::writeData(Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContextRef) {
    std::unique_lock lock(writeMutex);// TODO this is an anti-pattern in ZMQ
    connect();
    if (!connected) {
        x_DEBUG("ZmqSink: cannot write buffer because queue is not connected");
        throw Exceptions::RuntimeException("Write to zmq sink failed");
    }

    if (!inputBuffer) {
        x_ERROR("ZmqSink::writeData input buffer invalid");
        return false;
    }

    if (!schemaWritten && !internal) {//TODO:atomic
        x_DEBUG("FileSink::getData: write schema");
        auto fSchema = sinkFormat->getFormattedSchema();
        if (!fSchema.empty()) {
            x_DEBUG("ZmqSink writes schema buffer");
            try {
                // Send Header
                std::array<uint64_t, 2> const envelopeData{fSchema.size(), inputBuffer.getWatermark()};
                constexpr auto envelopeSize = sizeof(uint64_t) * 2;
                static_assert(envelopeSize == sizeof(envelopeData));
                zmq::message_t envelope{&(envelopeData[0]), envelopeSize};
                if (auto const sentEnvelopeSize = socket.send(envelope, zmq::send_flags::sndmore).value_or(0);
                    sentEnvelopeSize != envelopeSize) {
                    x_DEBUG("ZmqSink: schema send NOT successful");
                    return false;
                }

                // Send payload
                zmq::mutable_buffer payload{fSchema.data(), fSchema.size()};
                if (auto const sentPayloadSize = socket.send(payload, zmq::send_flags::none).value_or(0);
                    sentPayloadSize != payload.size()) {
                    x_DEBUG("ZmqSink: sending payload failed.");
                    return false;
                }
                x_DEBUG("ZmqSink: schema send successful");

            } catch (const zmq::error_t& ex) {
                if (ex.num() != ETERM) {
                    x_ERROR("ZmqSink: schema write  {}", ex.what());
                }
            }
            schemaWritten = true;
            x_DEBUG("ZmqSink::writeData: schema written");
        } else {
            x_DEBUG("ZmqSink::writeData: no schema written");
        }
    } else {
        x_DEBUG("ZmqSink::getData: schema already written");
    }

    auto buffer = sinkFormat->getFormattedBuffer(inputBuffer);
    x_DEBUG("ZmqSink: writes buffer with tupleCnt ={} watermark={} content=\n{}",
              inputBuffer.getNumberOfTuples(),
              inputBuffer.getWatermark(),
              buffer);
    try {
        ++sentBuffer;

        // Create envelope
        std::array<uint64_t, 2> const envelopeData{inputBuffer.getNumberOfTuples(), inputBuffer.getWatermark()};
        static_assert(sizeof(envelopeData) == sizeof(uint64_t) * 2);
        zmq::message_t envelope{&(envelopeData[0]), sizeof(envelopeData)};
        if (auto const sentEnvelope = socket.send(envelope, zmq::send_flags::sndmore).value_or(0);
            sentEnvelope != sizeof(envelopeData)) {
            x_WARNING("ZmqSink: data payload send NOT successful");
            return false;
        }

        // Create message.
        // Copying the entire payload here to avoid UB.
        zmq::message_t payload{buffer.data(), buffer.size()};
        if (auto const sentPayload = socket.send(payload, zmq::send_flags::none).value_or(0);
            sentPayload != inputBuffer.getBufferSize()) {
            x_WARNING("ZmqSink: data send NOT successful");
            return false;
        }
        x_DEBUG("ZmqSink: data send successful");
        return true;

    } catch (const zmq::error_t& ex) {
        // recv() throws ETERM when the zmq context is destroyed,
        //  as when AsyncZmqListener::Stop() is called
        if (ex.num() != ETERM) {
            x_ERROR("ZmqSink:  {}", ex.what());
        }
    }
    updateWatermarkCallback(inputBuffer);
    return true;
}

std::string ZmqSink::toString() const {
    std::stringstream ss;
    ss << "ZMQ_SINK(";
    ss << "SCHEMA(" << sinkFormat->getSchemaPtr()->toString() << "), ";
    ss << "HOST=" << host << ", ";
    ss << "PORT=" << port << ", ";
    ss << "INTERNAL=" << internal;
    ss << ")";
    return ss.str();
}

bool ZmqSink::connect() {
    if (!connected) {
        try {
            x_DEBUG("ZmqSink: connect to address= {}  port= {}", host, port);
            auto address = std::string("tcp://") + host + std::string(":") + std::to_string(port);
            socket.connect(address.c_str());
            connected = true;
        } catch (const zmq::error_t& ex) {
            // recv() throws ETERM when the zmq context is destroyed,
            //  as when AsyncZmqListener::Stop() is called
            if (ex.num() != ETERM) {
                x_ERROR("ZmqSink:  {}", ex.what());
            }
        }
    }
    if (connected) {
        x_DEBUG("ZmqSink : connected address= {}  port=  {}", host, port);
    } else {
        x_DEBUG("ZmqSink : NOT connected= {}  port=  {}", host, port);
    }
    return connected;
}

bool ZmqSink::disconnect() {
    if (connected) {
        socket.close();
        connected = false;
    }
    if (!connected) {
        x_DEBUG("ZmqSink : disconnected");
    } else {
        x_DEBUG("ZmqSink : NOT disconnected");
    }
    return !connected;
}

int ZmqSink::getPort() const { return this->port; }

std::string ZmqSink::getHost() const { return host; }

}// namespace x
