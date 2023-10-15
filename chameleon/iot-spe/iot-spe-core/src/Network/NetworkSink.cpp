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
#include <Network/NetworkSink.hpp>
#include <Runtime/Events.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Sinks/Formats/xFormat.hpp>
#include <Util/Common.hpp>
#include <Util/Core.hpp>

namespace x::Network {
NetworkSink::NetworkSink(const SchemaPtr& schema,
                         uint64_t uniqueNetworkSinkDescriptorId,
                         QueryId queryId,
                         QuerySubPlanId querySubPlanId,
                         const NodeLocation& destination,
                         xPartition xPartition,
                         Runtime::NodeEnginePtr nodeEngine,
                         size_t numOfProducers,
                         std::chrono::milliseconds waitTime,
                         uint8_t retryTimes,
                         FaultToleranceType faultToleranceType,
                         uint64_t numberOfOrigins)
    : SinkMedium(
        std::make_shared<xFormat>(schema, x::Util::checkNonNull(nodeEngine, "Invalid Node Engine")->getBufferManager()),
        nodeEngine,
        numOfProducers,
        queryId,
        querySubPlanId,
        faultToleranceType,
        numberOfOrigins,
        nullptr),
      uniqueNetworkSinkDescriptorId(uniqueNetworkSinkDescriptorId), nodeEngine(nodeEngine),
      networkManager(Util::checkNonNull(nodeEngine, "Invalid Node Engine")->getNetworkManager()),
      queryManager(Util::checkNonNull(nodeEngine, "Invalid Node Engine")->getQueryManager()), receiverLocation(destination),
      bufferManager(Util::checkNonNull(nodeEngine, "Invalid Node Engine")->getBufferManager()), xPartition(xPartition),
      numOfProducers(numOfProducers), waitTime(waitTime), retryTimes(retryTimes), reconnectBuffering(false) {
    x_ASSERT(this->networkManager, "Invalid network manager");
    x_DEBUG("NetworkSink: Created NetworkSink for partition {} location {}", xPartition, destination.createZmqURI());
    if (faultToleranceType == FaultToleranceType::AT_LEAST_ONCE) {
        insertIntoStorageCallback = [this](Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContext& workerContext) {
            workerContext.insertIntoStorage(this->xPartition, inputBuffer);
        };
    } else {
        insertIntoStorageCallback = [](Runtime::TupleBuffer&, Runtime::WorkerContext&) {
        };
    }
}

SinkMediumTypes NetworkSink::getSinkMediumType() { return SinkMediumTypes::NETWORK_SINK; }

bool NetworkSink::writeData(Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContext& workerContext) {
    //if a mobile node is in the process of reconnecting, do not attempt to send data but buffer it instead
    x_TRACE("context {} writing data", workerContext.getId());
    if (reconnectBuffering) {
        x_TRACE("context {} buffering data", workerContext.getId());
        workerContext.insertIntoStorage(this->xPartition, inputBuffer);
        return true;
    }

    auto* channel = workerContext.getNetworkChannel(xPartition.getOperatorId());
    if (channel) {
        auto success = channel->sendBuffer(inputBuffer, sinkFormat->getSchemaPtr()->getSchemaSizeInBytes());
        if (success) {
            insertIntoStorageCallback(inputBuffer, workerContext);
        }
        return success;
    }
    x_ASSERT2_FMT(false, "invalid channel on " << xPartition);
    return false;
}

void NetworkSink::preSetup() {
    x_DEBUG("NetworkSink: method preSetup() called {} qep {}", xPartition.toString(), querySubPlanId);
    x_ASSERT2_FMT(
        networkManager->registerSubpartitionEventConsumer(receiverLocation, xPartition, inherited1::shared_from_this()),
        "Cannot register event listener " << xPartition.toString());
}

void NetworkSink::setup() {
    x_DEBUG("NetworkSink: method setup() called {} qep {}", xPartition.toString(), querySubPlanId);
    auto reconf = Runtime::ReconfigurationMessage(queryId,
                                                  querySubPlanId,
                                                  Runtime::ReconfigurationType::Initialize,
                                                  inherited0::shared_from_this(),
                                                  std::make_any<uint32_t>(numOfProducers));
    queryManager->addReconfigurationMessage(queryId, querySubPlanId, reconf, true);
}

void NetworkSink::shutdown() {
    x_DEBUG("NetworkSink: shutdown() called {} queryId {} qepsubplan {}", xPartition.toString(), queryId, querySubPlanId);
    networkManager->unregisterSubpartitionProducer(xPartition);
}

std::string NetworkSink::toString() const { return "NetworkSink: " + xPartition.toString(); }

void NetworkSink::reconfigure(Runtime::ReconfigurationMessage& task, Runtime::WorkerContext& workerContext) {
    x_DEBUG("NetworkSink: reconfigure() called {} qep {}", xPartition.toString(), querySubPlanId);
    inherited0::reconfigure(task, workerContext);
    Runtime::QueryTerminationType terminationType = Runtime::QueryTerminationType::Invalid;
    switch (task.getType()) {
        case Runtime::ReconfigurationType::Initialize: {
            auto channel =
                networkManager->registerSubpartitionProducer(receiverLocation, xPartition, bufferManager, waitTime, retryTimes);
            x_ASSERT(channel, "Channel not valid partition " << xPartition);
            workerContext.storeNetworkChannel(xPartition.getOperatorId(), std::move(channel));
            workerContext.setObjectRefCnt(this, task.getUserData<uint32_t>());
            workerContext.createStorage(xPartition);
            x_DEBUG("NetworkSink: reconfigure() stored channel on {} Thread {} ref cnt {}",
                      xPartition.toString(),
                      Runtime::xThread::getId(),
                      task.getUserData<uint32_t>());
            break;
        }
        case Runtime::ReconfigurationType::HardEndOfStream: {
            terminationType = Runtime::QueryTerminationType::HardStop;
            break;
        }
        case Runtime::ReconfigurationType::SoftEndOfStream: {
            terminationType = Runtime::QueryTerminationType::Graceful;
            break;
        }
        case Runtime::ReconfigurationType::FailEndOfStream: {
            terminationType = Runtime::QueryTerminationType::Failure;
            break;
        }
        case Runtime::ReconfigurationType::PropagateEpoch: {
            auto* channel = workerContext.getNetworkChannel(xPartition.getOperatorId());
            //on arrival of an epoch barrier trim data in buffer storages in network sinks that belong to one query plan
            auto timestamp = task.getUserData<uint64_t>();
            x_DEBUG("Executing PropagateEpoch on qep queryId={} punctuation={}", queryId, timestamp);
            channel->sendEvent<Runtime::PropagateEpochEvent>(Runtime::EventType::kCustomEvent, timestamp, queryId);
            workerContext.trimStorage(xPartition, timestamp);
            break;
        }
        case Runtime::ReconfigurationType::StartBuffering: {
            //reconnect buffering is currently not supported if tuples are also buffered for fault tolerance
            //todo #3014: make reconnect buffering and fault tolerance buffering compatible
            if (faultToleranceType == FaultToleranceType::AT_LEAST_ONCE
                || faultToleranceType == FaultToleranceType::EXACTLY_ONCE) {
                break;
            }
            if (reconnectBuffering) {
                x_DEBUG("Requested sink to buffer but it is already buffering")
            } else {
                this->reconnectBuffering = true;
            }
            break;
        }
        case Runtime::ReconfigurationType::StopBuffering: {
            //reconnect buffering is currently not supported if tuples are also buffered for fault tolerance
            //todo #3014: make reconnect buffering and fault tolerance buffering compatible
            if (faultToleranceType == FaultToleranceType::AT_LEAST_ONCE
                || faultToleranceType == FaultToleranceType::EXACTLY_ONCE) {
                break;
            }
            /*stop buffering new incoming tuples. this will change the order of the tuples if new tuples arrive while we
            unbuffer*/
            reconnectBuffering = false;
            x_INFO("stop buffering data for context {}", workerContext.getId());
            auto topBuffer = workerContext.getTopTupleFromStorage(xPartition);
            x_INFO("sending buffered data");
            while (topBuffer) {
                /*this will only work if guarantees are not set to at least once,
                otherwise new tuples could be written to the buffer at the same time causing conflicting writes*/
                if (!topBuffer.value().getBuffer()) {
                    x_WARNING("buffer does not exist");
                    break;
                }
                if (!writeData(topBuffer.value(), workerContext)) {
                    x_WARNING("could not send all data from buffer");
                    break;
                }
                x_TRACE("buffer sent");
                workerContext.removeTopTupleFromStorage(xPartition);
                topBuffer = workerContext.getTopTupleFromStorage(xPartition);
            }
            break;
        }
        default: {
            break;
        }
    }
    if (terminationType != Runtime::QueryTerminationType::Invalid) {
        //todo #3013: make sure buffers are kept if the device is currently buffering
        if (workerContext.decreaseObjectRefCnt(this) == 1) {
            networkManager->unregisterSubpartitionProducer(xPartition);
            x_ASSERT2_FMT(workerContext.releaseNetworkChannel(xPartition.getOperatorId(), terminationType),
                            "Cannot remove network channel " << xPartition.toString());
            x_DEBUG("NetworkSink: reconfigure() released channel on {} Thread {}",
                      xPartition.toString(),
                      Runtime::xThread::getId());
        }
    }
}

void NetworkSink::postReconfigurationCallback(Runtime::ReconfigurationMessage& task) {
    x_DEBUG("NetworkSink: postReconfigurationCallback() called {} parent plan {}", xPartition.toString(), querySubPlanId);
    inherited0::postReconfigurationCallback(task);
}

void NetworkSink::onEvent(Runtime::BaseEvent& event) {
    x_DEBUG("NetworkSink::onEvent(event) called. uniqueNetworkSinkDescriptorId: {}", this->uniqueNetworkSinkDescriptorId);
    auto qep = queryManager->getQueryExecutionPlan(querySubPlanId);
    qep->onEvent(event);

    if (event.getEventType() == Runtime::EventType::kStartSourceEvent) {
        // todo jm continue here. how to obtain local worker context?
    }
}
void NetworkSink::onEvent(Runtime::BaseEvent& event, Runtime::WorkerContextRef) {
    x_DEBUG("NetworkSink::onEvent(event, wrkContext) called. uniqueNetworkSinkDescriptorId: {}",
              this->uniqueNetworkSinkDescriptorId);
    // this function currently has no usage
    onEvent(event);
}

OperatorId NetworkSink::getUniqueNetworkSinkDescriptorId() { return uniqueNetworkSinkDescriptorId; }

Runtime::NodeEnginePtr NetworkSink::getNodeEngine() { return nodeEngine; }

}// namespace x::Network
