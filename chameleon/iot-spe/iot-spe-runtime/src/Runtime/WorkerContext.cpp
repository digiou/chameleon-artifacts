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
#include <Runtime/BufferStorage.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/LocalBufferPool.hpp>
#include <Runtime/WorkerContext.hpp>

namespace x::Runtime {

folly::ThreadLocalPtr<LocalBufferPool> WorkerContext::localBufferPoolTLS{};

WorkerContext::WorkerContext(uint32_t workerId,
                             const BufferManagerPtr& bufferManager,
                             uint64_t numberOfBuffersPerWorker,
                             uint32_t queueId)
    : workerId(workerId), queueId(queueId) {
    //we changed from a local pool to a fixed sized pool as it allows us to manage the numbers that are hold in the cache via the paramter
    localBufferPool = bufferManager->createLocalBufferPool(numberOfBuffersPerWorker);
    localBufferPoolTLS.reset(localBufferPool.get(), [](auto*, folly::TLPDestructionMode) {
        // nop
    });
    x_ASSERT(!!localBufferPool, "Local buffer is not allowed to be null");
    x_ASSERT(!!localBufferPoolTLS, "Local buffer is not allowed to be null");
}

WorkerContext::~WorkerContext() {
    localBufferPool->destroy();
    localBufferPoolTLS.reset(nullptr);
}

uint32_t WorkerContext::getId() const { return workerId; }

uint32_t WorkerContext::getQueueId() const { return queueId; }

void WorkerContext::setObjectRefCnt(void* object, uint32_t refCnt) {
    objectRefCounters[reinterpret_cast<uintptr_t>(object)] = refCnt;
}

uint32_t WorkerContext::increaseObjectRefCnt(void* object) { return objectRefCounters[reinterpret_cast<uintptr_t>(object)]++; }

uint32_t WorkerContext::decreaseObjectRefCnt(void* object) {
    auto ptr = reinterpret_cast<uintptr_t>(object);
    if (auto it = objectRefCounters.find(ptr); it != objectRefCounters.end()) {
        auto val = it->second--;
        if (val == 1) {
            objectRefCounters.erase(it);
        }
        return val;
    }
    return 0;
}

TupleBuffer WorkerContext::allocateTupleBuffer() { return localBufferPool->getBufferBlocking(); }

void WorkerContext::storeNetworkChannel(x::OperatorId id, Network::NetworkChannelPtr&& channel) {
    x_TRACE("WorkerContext: storing channel for operator {}  for context {}", id, workerId);
    dataChannels[id] = std::move(channel);
}

void WorkerContext::createStorage(Network::xPartition xPartition) {
    this->storage[xPartition] = std::make_shared<BufferStorage>();
}

void WorkerContext::insertIntoStorage(Network::xPartition xPartition, x::Runtime::TupleBuffer buffer) {
    auto iteratorPartitionId = this->storage.find(xPartition);
    if (iteratorPartitionId != this->storage.end()) {
        this->storage[xPartition]->insertBuffer(buffer);
    } else {
        x_WARNING("No buffer storage found for partition {}, buffer was dropped", xPartition);
    }
}

void WorkerContext::trimStorage(Network::xPartition xPartition, uint64_t timestamp) {
    auto iteratorPartitionId = this->storage.find(xPartition);
    if (iteratorPartitionId != this->storage.end()) {
        this->storage[xPartition]->trimBuffer(timestamp);
    }
}

std::optional<x::Runtime::TupleBuffer> WorkerContext::getTopTupleFromStorage(Network::xPartition xPartition) {
    auto iteratorPartitionId = this->storage.find(xPartition);
    if (iteratorPartitionId != this->storage.end()) {
        return this->storage[xPartition]->getTopElementFromQueue();
    }
    return {};
}

void WorkerContext::removeTopTupleFromStorage(Network::xPartition xPartition) {
    auto iteratorPartitionId = this->storage.find(xPartition);
    if (iteratorPartitionId != this->storage.end()) {
        this->storage[xPartition]->removeTopElementFromQueue();
    }
}

bool WorkerContext::releaseNetworkChannel(x::OperatorId id, Runtime::QueryTerminationType terminationType) {
    x_TRACE("WorkerContext: releasing channel for operator {} for context {}", id, workerId);
    if (auto it = dataChannels.find(id); it != dataChannels.end()) {
        if (auto& channel = it->second; channel) {
            channel->close(terminationType);
        }
        dataChannels.erase(it);
        return true;
    }
    return false;
}

void WorkerContext::storeEventOnlyChannel(x::OperatorId id, Network::EventOnlyNetworkChannelPtr&& channel) {
    x_TRACE("WorkerContext: storing channel for operator {}  for context {}", id, workerId);
    reverseEventChannels[id] = std::move(channel);
}

bool WorkerContext::releaseEventOnlyChannel(x::OperatorId id, Runtime::QueryTerminationType terminationType) {
    x_TRACE("WorkerContext: releasing channel for operator {} for context {}", id, workerId);
    if (auto it = reverseEventChannels.find(id); it != reverseEventChannels.end()) {
        if (auto& channel = it->second; channel) {
            channel->close(terminationType);
        }
        reverseEventChannels.erase(it);
        return true;
    }
    return false;
}

Network::NetworkChannel* WorkerContext::getNetworkChannel(x::OperatorId ownerId) {
    x_TRACE("WorkerContext: retrieving channel for operator {} for context {}", ownerId, workerId);
    auto it = dataChannels.find(ownerId);// note we assume it's always available
    return (*it).second.get();
}

Network::EventOnlyNetworkChannel* WorkerContext::getEventOnlyNetworkChannel(x::OperatorId ownerId) {
    x_TRACE("WorkerContext: retrieving event only channel for operator {} for context {}", ownerId, workerId);
    auto it = reverseEventChannels.find(ownerId);// note we assume it's always available
    return (*it).second.get();
}

LocalBufferPool* WorkerContext::getBufferProviderTLS() { return localBufferPoolTLS.get(); }

LocalBufferPoolPtr WorkerContext::getBufferProvider() { return localBufferPool; }

}// namespace x::Runtime