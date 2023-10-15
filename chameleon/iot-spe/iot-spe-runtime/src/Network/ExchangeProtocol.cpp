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

#include <Network/ExchangeProtocol.hpp>
#include <Network/ExchangeProtocolListener.hpp>
#include <Network/PartitionManager.hpp>
#include <Runtime/Execution/DataEmitter.hpp>

namespace x::Network {

// Important invariant: never leak the protocolListener pointer
// there is a hack that disables the reference counting

ExchangeProtocol::ExchangeProtocol(std::shared_ptr<PartitionManager> partitionManager,
                                   std::shared_ptr<ExchangeProtocolListener> protocolListener)
    : partitionManager(std::move(partitionManager)), protocolListener(std::move(protocolListener)) {
    x_ASSERT(this->partitionManager, "Wrong parameter partitionManager is null");
    x_ASSERT(this->protocolListener, "Wrong parameter ExchangeProtocolListener is null");
    x_DEBUG("ExchangeProtocol: Initializing ExchangeProtocol()");
}

std::variant<Messages::ServerReadyMessage, Messages::ErrorMessage>
ExchangeProtocol::onClientAnnouncement(Messages::ClientAnnounceMessage msg) {
    using namespace Messages;
    // check if the partition is registered via the partition manager or wait until this is not done
    // if all good, send message back
    bool isDataChannel = msg.getMode() == Messages::ChannelType::DataChannel;
    x_INFO("ExchangeProtocol: ClientAnnouncement received for {} {}",
             msg.getChannelId().toString(),
             (isDataChannel ? "Data" : "EventOnly"));
    auto xPartition = msg.getChannelId().getxPartition();

    // check if identity is registered
    if (isDataChannel) {
        // we got a connection from a data channel: it means there is/will be a partition consumer running locally
        if (auto status = partitionManager->getConsumerRegistrationStatus(xPartition);
            status == PartitionRegistrationStatus::Registered) {
            // increment the counter
            partitionManager->pinSubpartitionConsumer(xPartition);
            x_DEBUG("ExchangeProtocol: ClientAnnouncement received for DataChannel {} REGISTERED",
                      msg.getChannelId().toString());
            // send response back to the client based on the identity
            return Messages::ServerReadyMessage(msg.getChannelId());
        } else if (status == PartitionRegistrationStatus::Deleted) {
            x_WARNING("ExchangeProtocol: ClientAnnouncement received for  DataChannel {} but WAS DELETED",
                        msg.getChannelId().toString());
            protocolListener->onServerError(Messages::ErrorMessage(msg.getChannelId(), ErrorType::DeletedPartitionError));
            return Messages::ErrorMessage(msg.getChannelId(), ErrorType::DeletedPartitionError);
        }
    } else {
        // we got a connection from an event-only channel: it means there is/will be an event consumer attached to a partition producer
        if (auto status = partitionManager->getProducerRegistrationStatus(xPartition);
            status == PartitionRegistrationStatus::Registered) {
            x_DEBUG("ExchangeProtocol: ClientAnnouncement received for EventChannel {} REGISTERED",
                      msg.getChannelId().toString());
            partitionManager->pinSubpartitionProducer(xPartition);
            // send response back to the client based on the identity
            return Messages::ServerReadyMessage(msg.getChannelId());
        } else if (status == PartitionRegistrationStatus::Deleted) {
            x_WARNING("ExchangeProtocol: ClientAnnouncement received for event channel {} WAS DELETED",
                        msg.getChannelId().toString());
            protocolListener->onServerError(Messages::ErrorMessage(msg.getChannelId(), ErrorType::DeletedPartitionError));
            return Messages::ErrorMessage(msg.getChannelId(), ErrorType::DeletedPartitionError);
        }
    }

    x_WARNING("ExchangeProtocol: ClientAnnouncement received for {} NOT REGISTERED", msg.getChannelId().toString());
    protocolListener->onServerError(Messages::ErrorMessage(msg.getChannelId(), ErrorType::PartitionNotRegisteredError));
    return Messages::ErrorMessage(msg.getChannelId(), ErrorType::PartitionNotRegisteredError);
}

void ExchangeProtocol::onBuffer(xPartition xPartition, Runtime::TupleBuffer& buffer) {
    if (partitionManager->getConsumerRegistrationStatus(xPartition) == PartitionRegistrationStatus::Registered) {
        protocolListener->onDataBuffer(xPartition, buffer);
        partitionManager->getDataEmitter(xPartition)->emitWork(buffer);
    } else {
        x_ERROR("DataBuffer for {} is not registered and was discarded!", xPartition.toString());
    }
}

void ExchangeProtocol::onServerError(const Messages::ErrorMessage error) { protocolListener->onServerError(error); }

void ExchangeProtocol::onChannelError(const Messages::ErrorMessage error) { protocolListener->onChannelError(error); }

void ExchangeProtocol::onEvent(xPartition xPartition, Runtime::BaseEvent& event) {
    if (partitionManager->getConsumerRegistrationStatus(xPartition) == PartitionRegistrationStatus::Registered) {
        protocolListener->onEvent(xPartition, event);
        partitionManager->getDataEmitter(xPartition)->onEvent(event);
    } else if (partitionManager->getProducerRegistrationStatus(xPartition) == PartitionRegistrationStatus::Registered) {
        protocolListener->onEvent(xPartition, event);
        if (auto listener = partitionManager->getEventListener(xPartition); listener != nullptr) {
            listener->onEvent(event);//
        }
    } else {
        x_ERROR("DataBuffer for {} is not registered and was discarded!", xPartition.toString());
    }
}

void ExchangeProtocol::onEndOfStream(Messages::EndOfStreamMessage endOfStreamMessage) {
    x_DEBUG("ExchangeProtocol: EndOfStream message received from {}", endOfStreamMessage.getChannelId().toString());
    if (partitionManager->getConsumerRegistrationStatus(endOfStreamMessage.getChannelId().getxPartition())
        == PartitionRegistrationStatus::Registered) {
        x_ASSERT2_FMT(!endOfStreamMessage.isEventChannel(),
                        "Received EOS for data channel on event channel for consumer "
                            << endOfStreamMessage.getChannelId().toString());
        if (partitionManager->unregisterSubpartitionConsumer(endOfStreamMessage.getChannelId().getxPartition())) {
            partitionManager->getDataEmitter(endOfStreamMessage.getChannelId().getxPartition())
                ->onEndOfStream(endOfStreamMessage.getQueryTerminationType());
            protocolListener->onEndOfStream(endOfStreamMessage);
        } else {
            x_DEBUG("ExchangeProtocol: EndOfStream message received on data channel from {} but there is still some active "
                      "subpartition: {}",
                      endOfStreamMessage.getChannelId().toString(),
                      *partitionManager->getSubpartitionConsumerCounter(endOfStreamMessage.getChannelId().getxPartition()));
        }
    } else if (partitionManager->getProducerRegistrationStatus(endOfStreamMessage.getChannelId().getxPartition())
               == PartitionRegistrationStatus::Registered) {
        x_ASSERT2_FMT(endOfStreamMessage.isEventChannel(),
                        "Received EOS for event channel on data channel for producer "
                            << endOfStreamMessage.getChannelId().toString());
        if (partitionManager->unregisterSubpartitionProducer(endOfStreamMessage.getChannelId().getxPartition())) {
            x_DEBUG("ExchangeProtocol: EndOfStream message received from event channel {} but with no active subpartition",
                      endOfStreamMessage.getChannelId().toString());
        } else {
            x_DEBUG("ExchangeProtocol: EndOfStream message received from event channel {} but there is still some active "
                      "subpartition: {}",
                      endOfStreamMessage.getChannelId().toString(),
                      *partitionManager->getSubpartitionProducerCounter(endOfStreamMessage.getChannelId().getxPartition()));
        }
    } else {
        x_ERROR("ExchangeProtocol: EndOfStream message received from {} however the partition is not registered on this worker",
                  endOfStreamMessage.getChannelId().toString());
        protocolListener->onServerError(
            Messages::ErrorMessage(endOfStreamMessage.getChannelId(), Messages::ErrorType::UnknownPartitionError));
    }
}

std::shared_ptr<PartitionManager> ExchangeProtocol::getPartitionManager() const { return partitionManager; }

}// namespace x::Network