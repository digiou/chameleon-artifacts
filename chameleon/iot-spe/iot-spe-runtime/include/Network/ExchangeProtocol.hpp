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

#ifndef x_RUNTIME_INCLUDE_NETWORK_EXCHANGEPROTOCOL_HPP_
#define x_RUNTIME_INCLUDE_NETWORK_EXCHANGEPROTOCOL_HPP_

#include <Network/NetworkMessage.hpp>
#include <variant>

namespace x::Runtime {
class BaseEvent;
}
namespace x::Network {
class PartitionManager;
class ExchangeProtocolListener;
/**
 * @brief This class is used by the ZmqServer and defix the reaction for events onDataBuffer,
 * clientAnnouncement, endOfStream and exceptionHandling between all nodes of x.
 */
class ExchangeProtocol {
  public:
    /**
     * @brief Create an exchange protocol object with a partition manager and a listener
     * @param partitionManager
     * @param listener
     */
    explicit ExchangeProtocol(std::shared_ptr<PartitionManager> partitionManager,
                              std::shared_ptr<ExchangeProtocolListener> listener);

    /**
     * @brief Reaction of the zmqServer after a ClientAnnounceMessage is received.
     * @param clientAnnounceMessage
     * @return if successful, return ServerReadyMessage
     */
    std::variant<Messages::ServerReadyMessage, Messages::ErrorMessage> onClientAnnouncement(Messages::ClientAnnounceMessage msg);

    /**
     * @brief Reaction of the zmqServer after a buffer is received.
     * @param id of the buffer
     * @param buffer content
     */
    void onBuffer(xPartition xPartition, Runtime::TupleBuffer& buffer);

    /**
     * @brief Reaction of the zmqServer after an error occurs.
     * @param the error message
     */
    void onServerError(Messages::ErrorMessage error);

    /**
     * @brief Reaction of the zmqServer after an error occurs.
     * @param the error message
     */
    void onChannelError(Messages::ErrorMessage error);

    /**
     * @brief Reaction of the zmqServer after an EndOfStream message is received.
     * @param the endOfStreamMessage
     */
    void onEndOfStream(Messages::EndOfStreamMessage endOfStreamMessage);

    /**
     * @brief This method is called when the server receives an event message.
     * @param xPartition
     * @param event
     */
    void onEvent(xPartition xPartition, Runtime::BaseEvent& event);

    /**
     * @brief getter for the PartitionManager
     * @return the PartitionManager
     */
    [[nodiscard]] std::shared_ptr<PartitionManager> getPartitionManager() const;

  private:
    std::shared_ptr<PartitionManager> partitionManager{nullptr};
    std::shared_ptr<ExchangeProtocolListener> protocolListener{nullptr};
};

}// namespace x::Network

#endif// x_RUNTIME_INCLUDE_NETWORK_EXCHANGEPROTOCOL_HPP_
