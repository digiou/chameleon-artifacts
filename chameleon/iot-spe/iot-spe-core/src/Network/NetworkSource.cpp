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

#include <Network/xPartition.hpp>
#include <Network/NetworkChannel.hpp>
#include <Network/NetworkManager.hpp>
#include <Network/NetworkSource.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>

namespace x::Network {

NetworkSource::NetworkSource(SchemaPtr schema,
                             Runtime::BufferManagerPtr bufferManager,
                             Runtime::QueryManagerPtr queryManager,
                             NetworkManagerPtr networkManager,
                             xPartition xPartition,
                             NodeLocation sinkLocation,
                             size_t numSourceLocalBuffers,
                             std::chrono::milliseconds waitTime,
                             uint8_t retryTimes,
                             std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors,
                             const std::string& physicalSourceName)

    : DataSource(std::move(schema),
                 std::move(bufferManager),
                 std::move(queryManager),
                 xPartition.getOperatorId(),
                 /*default origin id for the network source this is always zero*/ 0,
                 numSourceLocalBuffers,
                 GatheringMode::INTERVAL_MODE,
                 physicalSourceName,
                 std::move(successors)),
      networkManager(std::move(networkManager)), xPartition(xPartition), sinkLocation(std::move(sinkLocation)),
      waitTime(waitTime), retryTimes(retryTimes) {
    x_ASSERT(this->networkManager, "Invalid network manager");
}

std::optional<Runtime::TupleBuffer> NetworkSource::receiveData() {
    x_THROW_RUNTIME_ERROR("NetworkSource: ReceiveData() called, but method is invalid and should not be used.");
}

SourceType NetworkSource::getType() const { return SourceType::NETWORK_SOURCE; }

std::string NetworkSource::toString() const { return "NetworkSource: " + xPartition.toString(); }

// this is necessary to use std::visit below (see example: https://en.cppreference.com/w/cpp/utility/variant/visit)
namespace detail {
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}// namespace detail

bool NetworkSource::bind() {
    auto emitter = shared_from_base<DataEmitter>();
    return networkManager->registerSubpartitionConsumer(xPartition, sinkLocation, emitter);
}

bool NetworkSource::start() {
    using namespace Runtime;
    x_DEBUG("NetworkSource: start called on {}", xPartition);
    auto emitter = shared_from_base<DataEmitter>();
    auto expected = false;
    if (running.compare_exchange_strong(expected, true)) {
        for (const auto& successor : executableSuccessors) {
            auto querySubPlanId = std::visit(detail::overloaded{[](DataSinkPtr sink) {
                                                                    return sink->getParentPlanId();
                                                                },
                                                                [](Execution::ExecutablePipelinePtr pipeline) {
                                                                    return pipeline->getQuerySubPlanId();
                                                                }},
                                             successor);
            auto queryId = std::visit(detail::overloaded{[](DataSinkPtr sink) {
                                                             return sink->getQueryId();
                                                         },
                                                         [](Execution::ExecutablePipelinePtr pipeline) {
                                                             return pipeline->getQueryId();
                                                         }},
                                      successor);

            auto newReconf = ReconfigurationMessage(queryId,
                                                    querySubPlanId,
                                                    Runtime::ReconfigurationType::Initialize,
                                                    shared_from_base<DataSource>());
            queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, true);
            break;// hack as currently we assume only one executableSuccessor
        }
        x_DEBUG("NetworkSource: start completed on {}", xPartition);
        return true;
    }
    return false;
}

bool NetworkSource::fail() {
    using namespace Runtime;
    bool expected = true;
    if (running.compare_exchange_strong(expected, false)) {
        x_DEBUG("NetworkSource: fail called on {}", xPartition);
        auto newReconf =
            ReconfigurationMessage(-1, -1, ReconfigurationType::FailEndOfStream, DataSource::shared_from_base<DataSource>());
        queryManager->addReconfigurationMessage(-1, -1, newReconf, false);
        queryManager->notifySourceCompletion(shared_from_base<DataSource>(), Runtime::QueryTerminationType::Failure);
        return queryManager->addEndOfStream(shared_from_base<NetworkSource>(), Runtime::QueryTerminationType::Failure);
    }
    return false;
}

bool NetworkSource::stop(Runtime::QueryTerminationType type) {
    using namespace Runtime;
    bool expected = true;
    x_ASSERT2_FMT(type == QueryTerminationType::HardStop,
                    "NetworkSource::stop only supports HardStop or Failure :: partition " << xPartition);
    if (running.compare_exchange_strong(expected, false)) {
        x_DEBUG("NetworkSource: stop called on {}", xPartition);
        int invalidId = -1;
        auto newReconf = ReconfigurationMessage(invalidId,
                                                invalidId,
                                                ReconfigurationType::HardEndOfStream,
                                                DataSource::shared_from_base<DataSource>());
        queryManager->addReconfigurationMessage(invalidId, invalidId, newReconf, false);
        queryManager->notifySourceCompletion(shared_from_base<DataSource>(), Runtime::QueryTerminationType::HardStop);
        queryManager->addEndOfStream(shared_from_base<DataSource>(), Runtime::QueryTerminationType::HardStop);
        x_DEBUG("NetworkSource: stop called on {} sent hard eos", xPartition);
    } else {
        x_DEBUG("NetworkSource: stop called on {} but was already stopped", xPartition);
    }
    return true;
}

void NetworkSource::onEvent(Runtime::BaseEvent& event) {
    x_DEBUG("NetworkSource: received an event");
    if (event.getEventType() == Runtime::EventType::kCustomEvent) {
        auto epochEvent = dynamic_cast<Runtime::CustomEventWrapper&>(event).data<Runtime::PropagateEpochEvent>();
        auto epochBarrier = epochEvent->timestampValue();
        auto queryId = epochEvent->queryIdValue();
        auto success = queryManager->addEpochPropagation(shared_from_base<DataSource>(), queryId, epochBarrier);
        if (success) {
            x_DEBUG("NetworkSource::onEvent: epoch {} queryId {} propagated", epochBarrier, queryId);
        } else {
            x_ERROR("NetworkSource::onEvent:: could not propagate epoch {} queryId {}", epochBarrier, queryId);
        }
    }
}

void NetworkSource::reconfigure(Runtime::ReconfigurationMessage& task, Runtime::WorkerContext& workerContext) {
    x_DEBUG("NetworkSource: reconfigure() called {}", xPartition.toString());
    x::DataSource::reconfigure(task, workerContext);
    bool isTermination = false;
    Runtime::QueryTerminationType terminationType;
    switch (task.getType()) {
        case Runtime::ReconfigurationType::Initialize: {
            // we need to check again because between the invocations of
            // NetworkSource::start() and NetworkSource::reconfigure() the query might have
            // been stopped for some reasons
            if (networkManager->isPartitionConsumerRegistered(xPartition) == PartitionRegistrationStatus::Deleted) {
                return;
            }
            if (true)
                return;
            auto channel = networkManager->registerSubpartitionEventProducer(sinkLocation,
                                                                             xPartition,
                                                                             localBufferManager,
                                                                             waitTime,
                                                                             retryTimes);
            if (channel == nullptr) {
                x_DEBUG("NetworkSource: reconfigure() cannot get event channel {} on Thread {}",
                          xPartition.toString(),
                          Runtime::xThread::getId());
                return;// partition was deleted on the other side of the channel.. no point in waiting for a channel
            }
            workerContext.storeEventOnlyChannel(xPartition.getOperatorId(), std::move(channel));
            x_DEBUG("NetworkSource: reconfigure() stored event-channel {} Thread {}",
                      xPartition.toString(),
                      Runtime::xThread::getId());
            break;
        }
        case Runtime::ReconfigurationType::Destroy: {
            // necessary as event channel are lazily created so in the case of an immediate stop
            // they might not be established yet
            terminationType = Runtime::QueryTerminationType::Graceful;
            isTermination = true;
            break;
        }
        case Runtime::ReconfigurationType::HardEndOfStream: {
            terminationType = Runtime::QueryTerminationType::HardStop;
            isTermination = true;
        }
        case Runtime::ReconfigurationType::SoftEndOfStream: {
            terminationType = Runtime::QueryTerminationType::Graceful;
            isTermination = true;
            break;
        }
        default: {
            break;
        }
    }
    if (isTermination) {
        workerContext.releaseEventOnlyChannel(xPartition.getOperatorId(), terminationType);
        x_DEBUG("NetworkSource: reconfigure() released channel on {} Thread {}",
                  xPartition.toString(),
                  Runtime::xThread::getId());
    }
}

void NetworkSource::postReconfigurationCallback(Runtime::ReconfigurationMessage& task) {
    x_DEBUG("NetworkSource: postReconfigurationCallback() called {}", xPartition.toString());
    x::DataSource::postReconfigurationCallback(task);
    Runtime::QueryTerminationType terminationType = Runtime::QueryTerminationType::Invalid;
    switch (task.getType()) {
        case Runtime::ReconfigurationType::FailEndOfStream: {
            terminationType = Runtime::QueryTerminationType::Failure;
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
        default: {
            break;
        }
    }
    if (Runtime::QueryTerminationType::Invalid != terminationType) {
        x_DEBUG("NetworkSource: postReconfigurationCallback(): unregistering SubpartitionConsumer {}", xPartition.toString());
        networkManager->unregisterSubpartitionConsumer(xPartition);
        bool expected = true;
        if (running.compare_exchange_strong(expected, false)) {
            x_DEBUG("NetworkSource is stopped on reconf task with id {}", xPartition.toString());
            queryManager->notifySourceCompletion(shared_from_base<DataSource>(), terminationType);
        }
    }
}

void NetworkSource::runningRoutine(const Runtime::BufferManagerPtr&, const Runtime::QueryManagerPtr&) {
    x_THROW_RUNTIME_ERROR("NetworkSource: runningRoutine() called, but method is invalid and should not be used.");
}
void NetworkSource::onEndOfStream(Runtime::QueryTerminationType terminationType) {
    // propagate EOS to the locally running QEPs that use the network source
    x_DEBUG("Going to inject eos for {} terminationType={}", xPartition, terminationType);
    if (Runtime::QueryTerminationType::Graceful == terminationType) {
        queryManager->addEndOfStream(shared_from_base<DataSource>(), Runtime::QueryTerminationType::Graceful);
    } else {
        x_WARNING("Ignoring forceful EoS on {}", xPartition);
    }
}

void NetworkSource::onEvent(Runtime::BaseEvent& event, Runtime::WorkerContextRef workerContext) {
    x_DEBUG("NetworkSource::onEvent(event, wrkContext) called. operatorId: {}", this->operatorId);
    if (event.getEventType() == Runtime::EventType::kStartSourceEvent) {
        auto senderChannel = workerContext.getEventOnlyNetworkChannel(this->operatorId);
        senderChannel->sendEvent<Runtime::StartSourceEvent>();
    }
}

}// namespace x::Network