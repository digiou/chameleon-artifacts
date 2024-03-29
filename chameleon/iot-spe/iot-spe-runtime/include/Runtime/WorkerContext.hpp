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

#ifndef x_RUNTIME_INCLUDE_RUNTIME_WORKERCONTEXT_HPP_
#define x_RUNTIME_INCLUDE_RUNTIME_WORKERCONTEXT_HPP_

#include <Network/xPartition.hpp>
#include <Network/NetworkForwardRefs.hpp>
#include <Runtime/QueryTerminationType.hpp>
#include <Runtime/RuntimeForwardRefs.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <cstdint>
#include <folly/ThreadLocal.h>
#include <memory>
#include <queue>
#include <unordered_map>

namespace x::Runtime {

class AbstractBufferProvider;
class BufferStorage;
using BufferStoragePtr = std::shared_ptr<Runtime::BufferStorage>;

/**
 * @brief A WorkerContext represents the current state of a worker thread
 * Note that it is not thread-safe per se but it is meant to be used in
 * a thread-safe manner by the ThreadPool.
 */
class WorkerContext {
  private:
    using WorkerContextBufferProviderPtr = LocalBufferPoolPtr;
    using WorkerContextBufferProvider = WorkerContextBufferProviderPtr::element_type;
    using WorkerContextBufferProviderRawPtr = WorkerContextBufferProviderPtr::element_type*;

    /// the id of this worker context (unique per thread).
    uint32_t workerId;
    /// object reference counters
    std::unordered_map<uintptr_t, uint32_t> objectRefCounters;
    /// data channels that send data downstream
    std::unordered_map<x::OperatorId, Network::NetworkChannelPtr> dataChannels;
    /// event only channels that send events upstream
    std::unordered_map<x::OperatorId, Network::EventOnlyNetworkChannelPtr> reverseEventChannels;
    /// worker local buffer pool stored in tls
    static folly::ThreadLocalPtr<WorkerContextBufferProvider> localBufferPoolTLS;
    /// worker local buffer pool stored :: use this for fast access
    WorkerContextBufferProviderPtr localBufferPool;
    /// numa location of current worker
    uint32_t queueId = 0;
    std::unordered_map<Network::xPartition, BufferStoragePtr> storage;

  public:
    explicit WorkerContext(uint32_t workerId,
                           const BufferManagerPtr& bufferManager,
                           uint64_t numberOfBuffersPerWorker,
                           uint32_t queueId = 0);

    ~WorkerContext();

    /**
     * @brief Allocates a new tuple buffer.
     * @return TupleBuffer
     */
    TupleBuffer allocateTupleBuffer();

    /**
     * @brief Returns the thread-local buffer provider singleton.
     * This can be accessed at any point in time also without the pointer to the context.
     * Calling this method from a non worker thread results in undefined behaviour.
     * @return raw pointer to AbstractBufferProvider
     */
    static WorkerContextBufferProviderRawPtr getBufferProviderTLS();

    /**
     * @brief Returns the thread-local buffer provider
     * @return shared_ptr to LocalBufferPool
     */
    WorkerContextBufferProviderPtr getBufferProvider();

    /**
     * @brief get current worker context thread id. This is assigned by calling xThread::getId()
     * @return current worker context thread id
     */
    uint32_t getId() const;

    /**
     * @brief Sets the ref counter for a generic object using its pointer address as lookup
     * @param object the object that we want to track
     * @param refCnt the initial ref cnt
     */
    void setObjectRefCnt(void* object, uint32_t refCnt);

    /**
     * @brief Increase the ref cnt of a given object
     * @param object the object that we want to ref count
     * @return the prev ref cnt
     */
    uint32_t increaseObjectRefCnt(void* object);

    /**
     * @brief Reduces by one the ref cnt. It deletes the object as soon as ref cnt reaches 0.
     * @param object the object that we want to ref count
     * @return the prev ref cnt
     */
    uint32_t decreaseObjectRefCnt(void* object);

    /**
     * @brief get the queue id of the the current worker
     * @return current queue id
     */
    uint32_t getQueueId() const;

    /**
     * @brief This stores a network channel for an operator
     * @param id of the operator that we want to store the output channel
     * @param channel the output channel
     */
    void storeNetworkChannel(x::OperatorId id, Network::NetworkChannelPtr&& channel);

    /**
      * @brief This method creates a network storage for a thread
      * @param xPartitionId partition
      */
    void createStorage(Network::xPartition xPartition);

    /**
      * @brief This method inserts a tuple buffer into the storage
      * @param xPartition partition
      * @param TupleBuffer tuple buffer
      */
    void insertIntoStorage(Network::xPartition xPartition, x::Runtime::TupleBuffer buffer);

    /**
      * @brief This method deletes a tuple buffer from the storage
      * @param xPartition partition
      * @param timestamp timestamp
      */
    void trimStorage(Network::xPartition xPartition, uint64_t timestamp);

    /**
     * @brief get the oldest buffered tuple for the specified partition
     * @param xPartition partition
     * @return an optional containing the tuple or nullopt if the storage is empty
     */
    std::optional<x::Runtime::TupleBuffer> getTopTupleFromStorage(Network::xPartition xPartition);

    /**
     * @brief if the storage is not empty remove the oldest buffered tuple for the specified partition
     * @param xPartition partition
     */
    void removeTopTupleFromStorage(Network::xPartition xPartition);

    /**
     * @brief removes a registered network channel with a termination type
     * @param id of the operator that we want to store the output channel
     * @param type the termination type
     */
    bool releaseNetworkChannel(x::OperatorId id, Runtime::QueryTerminationType type);

    /**
     * @brief This stores a network channel for an operator
     * @param id of the operator that we want to store the output channel
     * @param channel the output channel
     */
    void storeEventOnlyChannel(x::OperatorId id, Network::EventOnlyNetworkChannelPtr&& channel);

    /**
     * @brief removes a registered network channel
     * @param id of the operator that we want to store the output channel
     * @param terminationType the termination type
     */
    bool releaseEventOnlyChannel(x::OperatorId id, Runtime::QueryTerminationType terminationType);

    /**
     * @brief retrieve a registered output channel
     * @param ownerId id of the operator that we want to store the output channel
     * @return an output channel
     */
    Network::NetworkChannel* getNetworkChannel(x::OperatorId ownerId);

    /**
     * @brief retrieve a registered output channel
     * @param ownerId id of the operator that we want to store the output channel
     * @return an output channel
     */
    Network::EventOnlyNetworkChannel* getEventOnlyNetworkChannel(x::OperatorId ownerId);
};
using WorkerContextPtr = std::shared_ptr<WorkerContext>;
}// namespace x::Runtime
#endif// x_RUNTIME_INCLUDE_RUNTIME_WORKERCONTEXT_HPP_
