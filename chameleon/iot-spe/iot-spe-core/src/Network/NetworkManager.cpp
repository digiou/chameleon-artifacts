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

#include <Network/NetworkChannel.hpp>
#include <Network/NetworkManager.hpp>
#include <Network/PartitionManager.hpp>
#include <Network/ZmqServer.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Network {

NetworkManager::NetworkManager(uint64_t nodeEngineId,
                               const std::string& hostname,
                               uint16_t port,
                               ExchangeProtocol&& exchangeProtocol,
                               const Runtime::BufferManagerPtr& bufferManager,
                               int senderHighWatermark,
                               uint16_t numServerThread)
    : nodeLocation(), server(std::make_unique<ZmqServer>(hostname, port, numServerThread, this->exchangeProtocol, bufferManager)),
      exchangeProtocol(std::move(exchangeProtocol)), partitionManager(this->exchangeProtocol.getPartitionManager()),
      senderHighWatermark(senderHighWatermark) {

    if (bool const success = server->start(); success) {
        nodeLocation = NodeLocation(nodeEngineId, hostname, server->getServerPort());
        x_INFO("NetworkManager: Server started successfully on {}", nodeLocation.createZmqURI());
    } else {
        x_THROW_RUNTIME_ERROR("NetworkManager: Server failed to start on " << hostname << ":" << port);
    }
}

NetworkManager::~NetworkManager() { destroy(); }

NetworkManagerPtr NetworkManager::create(uint64_t nodeEngineId,
                                         const std::string& hostname,
                                         uint16_t port,
                                         Network::ExchangeProtocol&& exchangeProtocol,
                                         const Runtime::BufferManagerPtr& bufferManager,
                                         int senderHighWatermark,
                                         uint16_t numServerThread) {
    return std::make_shared<NetworkManager>(nodeEngineId,
                                            hostname,
                                            port,
                                            std::move(exchangeProtocol),
                                            bufferManager,
                                            senderHighWatermark,
                                            numServerThread);
}

void NetworkManager::destroy() { server->stop(); }

PartitionRegistrationStatus NetworkManager::isPartitionConsumerRegistered(const xPartition& xPartition) const {
    return partitionManager->getConsumerRegistrationStatus(xPartition);
}

PartitionRegistrationStatus NetworkManager::isPartitionProducerRegistered(const xPartition& xPartition) const {
    return partitionManager->getProducerRegistrationStatus(xPartition);
}

NodeLocation NetworkManager::getServerLocation() const { return nodeLocation; }

uint16_t NetworkManager::getServerDataPort() const { return server->getServerPort(); }

bool NetworkManager::registerSubpartitionConsumer(const xPartition& xPartition,
                                                  const NodeLocation& senderLocation,
                                                  const DataEmitterPtr& emitter) const {
    x_DEBUG("NetworkManager: Registering SubpartitionConsumer: {} from {}",
              xPartition.toString(),
              senderLocation.getHostname());
    x_ASSERT2_FMT(emitter, "invalid network source " << xPartition.toString());
    return partitionManager->registerSubpartitionConsumer(xPartition, senderLocation, emitter);
}

bool NetworkManager::unregisterSubpartitionConsumer(const xPartition& xPartition) const {
    x_DEBUG("NetworkManager: Unregistering SubpartitionConsumer: {}", xPartition.toString());
    return partitionManager->unregisterSubpartitionConsumer(xPartition);
}

bool NetworkManager::unregisterSubpartitionProducer(const xPartition& xPartition) const {
    x_DEBUG("NetworkManager: Unregistering SubpartitionProducer: {}", xPartition.toString());
    return partitionManager->unregisterSubpartitionProducer(xPartition);
}

NetworkChannelPtr NetworkManager::registerSubpartitionProducer(const NodeLocation& nodeLocation,
                                                               const xPartition& xPartition,
                                                               Runtime::BufferManagerPtr bufferManager,
                                                               std::chrono::milliseconds waitTime,
                                                               uint8_t retryTimes) {
    x_DEBUG("NetworkManager: Registering SubpartitionProducer: {}", xPartition.toString());
    partitionManager->registerSubpartitionProducer(xPartition, nodeLocation);
    return NetworkChannel::create(server->getContext(),
                                  nodeLocation.createZmqURI(),
                                  xPartition,
                                  exchangeProtocol,
                                  std::move(bufferManager),
                                  senderHighWatermark,
                                  waitTime,
                                  retryTimes);
}

EventOnlyNetworkChannelPtr NetworkManager::registerSubpartitionEventProducer(const NodeLocation& nodeLocation,
                                                                             const xPartition& xPartition,
                                                                             Runtime::BufferManagerPtr bufferManager,
                                                                             std::chrono::milliseconds waitTime,
                                                                             uint8_t retryTimes) {
    x_DEBUG("NetworkManager: Registering SubpartitionEvent Producer: {}", xPartition.toString());
    return EventOnlyNetworkChannel::create(server->getContext(),
                                           nodeLocation.createZmqURI(),
                                           xPartition,
                                           exchangeProtocol,
                                           std::move(bufferManager),
                                           senderHighWatermark,
                                           waitTime,
                                           retryTimes);
}

bool NetworkManager::registerSubpartitionEventConsumer(const NodeLocation& nodeLocation,
                                                       const xPartition& xPartition,
                                                       Runtime::RuntimeEventListenerPtr eventListener) {
    // note that this increases the subpartition producer counter by one
    // we want to do so to keep the partition alive until all outbound network channel + the inbound event channel are in-use
    x_DEBUG("NetworkManager: Registering Subpartition Event Consumer: {}", xPartition.toString());
    return partitionManager->addSubpartitionEventListener(xPartition, nodeLocation, eventListener);
}

}// namespace x::Network