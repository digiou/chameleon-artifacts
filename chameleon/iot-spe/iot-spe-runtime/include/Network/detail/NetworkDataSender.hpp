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

#ifndef x_RUNTIME_INCLUDE_NETWORK_DETAIL_NETWORKDATASENDER_HPP_
#define x_RUNTIME_INCLUDE_NETWORK_DETAIL_NETWORKDATASENDER_HPP_

#include <Network/ZmqUtils.hpp>
#include <Runtime/BufferManager.hpp>
namespace x::Network::detail {

/**
 * @brief Mixin to add sendBuffer semantics to the BaseChannelType
 * @tparam BaseChannelType the base channel type
 */
template<typename BaseChannelType>
class NetworkDataSender : public BaseChannelType {
  public:
    static constexpr bool canSendData = true;
    static constexpr bool canSendEvent = false || BaseChannelType::canSendEvent;

    /**
     * @brief Forwarding ctor: it forwards the args to the base class
     * @tparam Args the arguments types
     * @param args the arguments
     */
    template<typename... Args>
    NetworkDataSender(Args&&... args) : BaseChannelType(std::forward<Args>(args)...) {}

    /**
     * @brief Send buffer to the destination defined in the constructor. Note that this method will internally
     * compute the payloadSize as tupleSizeInBytes*buffer.getNumberOfTuples()
     * @param the inputBuffer to send
     * @param the tupleSize represents the size in bytes of one tuple in the buffer
     * @return true if send was successful, else false
     */
    bool sendBuffer(Runtime::TupleBuffer& inputBuffer, uint64_t tupleSize) {
        auto numOfTuples = inputBuffer.getNumberOfTuples();
        auto originId = inputBuffer.getOriginId();
        auto watermark = inputBuffer.getWatermark();
        auto sequenceNumber = inputBuffer.getSequenceNumber();
        auto creationTimestamp = inputBuffer.getCreationTimestampInMS();
        auto payloadSize = tupleSize * numOfTuples;
        auto* ptr = inputBuffer.getBuffer<uint8_t>();
        auto numOfChildren = inputBuffer.getNumberOfChildrenBuffer();
        if (payloadSize == 0) {
            return true;
        }

        sendMessage<Messages::DataBufferMessage, kZmqSendMore>(this->zmqSocket,
                                                               payloadSize,
                                                               numOfTuples,
                                                               originId,
                                                               watermark,
                                                               creationTimestamp,
                                                               sequenceNumber,
                                                               numOfChildren);

        bool res = true;
        for (auto i = 0u; i < numOfChildren; ++i) {
            auto childBuffer = inputBuffer.loadChildBuffer(i);
            // We need to retain the `childBuffer` here, because the send function operates asynchronously and we therefore
            // need to pass the responsibility of freeing the tupleBuffer instance to ZMQ's callback.
            childBuffer.retain();
            sendMessageNoHeader<Messages::DataBufferMessage, kZmqSendMore>(this->zmqSocket,
                                                                           childBuffer.getBufferSize(),
                                                                           1,
                                                                           originId,
                                                                           watermark,
                                                                           creationTimestamp,
                                                                           sequenceNumber,
                                                                           0);
            auto const sentBytesOpt = this->zmqSocket.send(zmq::message_t(childBuffer.getBuffer(),
                                                                          childBuffer.getBufferSize(),
                                                                          &Runtime::detail::zmqBufferRecyclingCallback,
                                                                          childBuffer.getControlBlock()),
                                                           kZmqSendMore);
            res &= !!sentBytesOpt;
            x_TRACE("Sending child #{} was {}", i, ((!!sentBytesOpt) ? " successful " : " not successful"));
        }

        if (!res) {
            x_ERROR("DataChannel: Error sending children buffer for {}", this->channelId);
            return false;
        }

        // again, we need to retain the `inputBuffer` here, because the send function operates asynchronously and we therefore
        // need to pass the responsibility of freeing the tupleBuffer instance to ZMQ's callback.
        inputBuffer.retain();
        auto const sentBytesOpt = this->zmqSocket.send(
            zmq::message_t(ptr, payloadSize, &Runtime::detail::zmqBufferRecyclingCallback, inputBuffer.getControlBlock()),
            kZmqSendDefault);
        if (!!sentBytesOpt) {
            x_TRACE("DataChannel: Sending buffer with {}/{}-{}",
                      inputBuffer.getNumberOfTuples(),
                      inputBuffer.getBufferSize(),
                      inputBuffer.getOriginId());

            return true;
        }

        x_ERROR("DataChannel: Error sending buffer for {}", this->channelId);
        return false;
    }
};

}// namespace x::Network::detail
#endif// x_RUNTIME_INCLUDE_NETWORK_DETAIL_NETWORKDATASENDER_HPP_
