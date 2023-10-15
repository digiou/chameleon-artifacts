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

#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/QueryManager.hpp>
#include <Sources/ZmqSource.hpp>
#include <Util/Logger/Logger.hpp>
#include <cstdint>
#include <cstring>

#include <sstream>
#include <string>
#include <utility>
#include <zmq.hpp>
namespace x {

ZmqSource::ZmqSource(SchemaPtr schema,
                     Runtime::BufferManagerPtr bufferManager,
                     Runtime::QueryManagerPtr queryManager,
                     const std::string& host,
                     uint16_t port,
                     OperatorId operatorId,
                     OriginId originId,
                     uint64_t numSourceLocalBuffers,
                     GatheringMode gatheringMode,
                     const std::string& physicalSourceName,
                     std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors)
    : DataSource(std::move(schema),
                 std::move(bufferManager),
                 std::move(queryManager),
                 operatorId,
                 originId,
                 numSourceLocalBuffers,
                 gatheringMode,
                 physicalSourceName,
                 std::move(successors)),
      host(host), port(port), connected(false), context(zmq::context_t(1)), socket(zmq::socket_t(context, ZMQ_PULL)) {
    x_DEBUG("ZMQSOURCE: Init ZMQ ZMQSOURCE to  {} : {} /", host, port);
}

ZmqSource::~ZmqSource() x_NOEXCEPT(false) {
    x_DEBUG("ZmqSource::~ZmqSource()");
    bool success = disconnect();
    if (success) {
        x_DEBUG("ZMQSOURCE: Destroy ZMQ Source");
    } else {
        x_ASSERT2_FMT(false, "ZMQSOURCE  " << this << ": Destroy ZMQ Source failed cause it could not be disconnected");
    }
    x_DEBUG("ZMQSOURCE: Destroy ZMQ Source");
}

std::optional<Runtime::TupleBuffer> ZmqSource::receiveData() {
    x_DEBUG("ZMQSource: receiveData ", this->toString());
    if (connect()) {
        try {

            // Receive metadata
            auto const metadataSize = sizeof(uint64_t) * 2;
            zmq::message_t metadata{metadataSize};

            // TODO: Clarify following comment: envelope - not needed at the moment
            if (auto const receivedSize = socket.recv(metadata).value_or(0); receivedSize != metadataSize) {
                x_ERROR("ZMQSource: Error: Unexpected payload size. Expected: {} Received: {}", metadataSize, receivedSize);
                return std::nullopt;
            }

            auto buffer = bufferManager->getBufferBlocking();
            buffer.setNumberOfTuples(static_cast<uint64_t*>(metadata.data())[0]);
            buffer.setWatermark(static_cast<uint64_t*>(metadata.data())[1]);
            x_DEBUG("ZMQSource received #tups  {}  watermark= {}", buffer.getNumberOfTuples(), buffer.getWatermark());

            // Receive payload
            // XXX: I guess we don't actually know the size here, it would be nice to be able to check that here
            zmq::mutable_buffer payload{buffer.getBuffer(), buffer.getBufferSize()};
            if (auto const receivedSize = socket.recv(payload); !receivedSize.has_value()) {
                x_ERROR("ZMQSource: Error: Unexpected payload size. Expected: {} Received: {}",
                          buffer.getBufferSize(),
                          receivedSize.has_value());
                return std::nullopt;
            } else {
                x_DEBUG("ZMQSource: received buffer of size  {}", receivedSize.has_value());
                return buffer;
            }

        } catch (const zmq::error_t& ex) {
            x_ERROR("ZMQSOURCE error: {}", ex.what());
            return std::nullopt;
        } catch (...) {
            x_ERROR("ZMQSOURCE general error");
            return std::nullopt;
        }
    } else {
        x_ERROR("ZMQSOURCE: Not connected!");
        return std::nullopt;
    }
}

std::string ZmqSource::toString() const {
    std::stringstream ss;
    ss << "ZMQ_SOURCE(";
    ss << "SCHEMA(" << schema->toString() << "), ";
    ss << "HOST=" << host << ", ";
    ss << "PORT=" << port << ", ";
    return ss.str();
}

bool ZmqSource::connect() {
    if (!connected) {
        x_DEBUG("ZMQSOURCE was !conncect now connect: connected");
        if (host == "localhost") {
            host = "*";
        }
        auto address = std::string("tcp://") + host + std::string(":") + std::to_string(port);
        x_DEBUG("ZMQSOURCE use address {}", address);
        try {
            socket.set(zmq::sockopt::linger, 0);
            socket.bind(address.c_str());
            x_DEBUG("ZMQSOURCE: set connected true");
            connected = true;
        } catch (const zmq::error_t& ex) {
            // recv() throws ETERM when the zmq context is destroyed,
            //  as when AsyncZmqListener::Stop() is called
            if (ex.num() != ETERM) {
                x_ERROR("ZMQSOURCE ERROR: {}", ex.what());
                x_DEBUG("ZMQSOURCE: set connected false");
            }
            connected = false;
        }
    }

    if (connected) {
        x_DEBUG("ZMQSOURCE: connected");
    } else {
        x_DEBUG("Exception: ZMQSOURCE: NOT connected");
    }
    return connected;
}

bool ZmqSource::disconnect() {
    x_DEBUG("ZmqSource::disconnect() connected={}", connected);
    if (connected) {
        // we put assert here because it d be called anyway from the shutdown method
        // that we commented out
        bool success = zmq_ctx_shutdown(static_cast<void*>(context)) == 0;
        if (!success) {
            throw Exceptions::RuntimeException("ZmqSource::disconnect() error");
        }
        //        context.shutdown();
        connected = false;
    }
    if (!connected) {
        x_DEBUG("ZMQSOURCE: disconnected");
    } else {
        x_DEBUG("ZMQSOURCE: NOT disconnected");
    }
    return !connected;
}

SourceType ZmqSource::getType() const { return SourceType::ZMQ_SOURCE; }

const std::string& ZmqSource::getHost() const { return host; }

uint16_t ZmqSource::getPort() const { return port; }

}// namespace x