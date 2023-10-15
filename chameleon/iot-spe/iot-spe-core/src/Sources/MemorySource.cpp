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
#include <Runtime/NodeEngine.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/internal/apex_memmove.hpp>
#ifdef __x86_64__
#include <Runtime/internal/rte_memory.h>
#endif
#include <Sources/MemorySource.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <cmath>
#ifdef x_USE_ONE_QUEUE_PER_NUMA_NODE
#if defined(__linux__)
#include <numa.h>
#include <numaif.h>
#endif
#endif
#include <Util/magicenum/magic_enum.hpp>
#include <utility>

namespace x {

MemorySource::MemorySource(SchemaPtr schema,
                           const std::shared_ptr<uint8_t>& memoryArea,
                           size_t memoryAreaSize,
                           Runtime::BufferManagerPtr bufferManager,
                           Runtime::QueryManagerPtr queryManager,
                           uint64_t numBuffersToProcess,
                           uint64_t gatheringValue,
                           OperatorId operatorId,
                           OriginId originId,
                           size_t numSourceLocalBuffers,
                           GatheringMode gatheringMode,
                           uint64_t sourceAffinity,
                           uint64_t taskQueueId,
                           const std::string& physicalSourceName,
                           std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors)
    : GeneratorSource(std::move(schema),
                      std::move(bufferManager),
                      std::move(queryManager),
                      numBuffersToProcess,
                      operatorId,
                      originId,
                      numSourceLocalBuffers,
                      gatheringMode,
                      std::move(successors),
                      physicalSourceName),
      memoryArea(memoryArea), memoryAreaSize(memoryAreaSize), currentPositionInBytes(0) {
    this->numberOfBuffersToProduce = numBuffersToProcess;
    if (gatheringMode == GatheringMode::INTERVAL_MODE) {
        this->gatheringInterval = std::chrono::milliseconds(gatheringValue);
    } else if (gatheringMode == GatheringMode::INGESTION_RATE_MODE) {
        this->gatheringIngestionRate = gatheringValue;
    } else {
        x_THROW_RUNTIME_ERROR("Mode not implemented " << magic_enum::enum_name(gatheringMode));
    }
    this->sourceAffinity = sourceAffinity;
    schemaSize = this->schema->getSchemaSizeInBytes();
    bufferSize = localBufferManager->getBufferSize();

    this->sourceAffinity = sourceAffinity;
    this->taskQueueId = taskQueueId;

    //if the memory area is smaller than a buffer
    if (memoryAreaSize <= bufferSize) {
        numberOfTuplesToProduce = std::floor(double(memoryAreaSize) / double(this->schemaSize));
    } else {
        //if the memory area spans multiple buffers
        auto restTuples = (memoryAreaSize - currentPositionInBytes) / this->schemaSize;
        auto numberOfTuplesPerBuffer = std::floor(double(bufferSize) / double(this->schemaSize));
        if (restTuples > numberOfTuplesPerBuffer) {
            numberOfTuplesToProduce = numberOfTuplesPerBuffer;
        } else {
            numberOfTuplesToProduce = restTuples;
        }
    }

    x_DEBUG("MemorySource() numBuffersToProcess= {}  memoryAreaSize= {}", numBuffersToProcess, memoryAreaSize);
    x_ASSERT(memoryArea && memoryAreaSize > 0, "invalid memory area");
}

std::optional<Runtime::TupleBuffer> MemorySource::receiveData() {
    x_DEBUG("MemorySource::receiveData called on operatorId={}", operatorId);
    if (memoryAreaSize > bufferSize) {
        if (currentPositionInBytes + numberOfTuplesToProduce * schemaSize > memoryAreaSize) {
            if (numberOfBuffersToProduce != 0) {
                x_DEBUG("MemorySource::receiveData: reset buffer to 0");
                currentPositionInBytes = 0;
            } else {
                x_DEBUG("MemorySource::receiveData: return as mem sry is empty");
                return std::nullopt;
            }
        }
    }

    x_ASSERT2_FMT(numberOfTuplesToProduce * schemaSize <= bufferSize, "value to write is larger than the buffer");

    Runtime::TupleBuffer buffer = bufferManager->getBufferBlocking();
    memcpy(buffer.getBuffer(), memoryArea.get() + currentPositionInBytes, bufferSize);

    if (memoryAreaSize > bufferSize) {
        x_TRACE("MemorySource::receiveData: add offset={} to currentpos={}", bufferSize, currentPositionInBytes);
        currentPositionInBytes += bufferSize;
    }

    buffer.setNumberOfTuples(numberOfTuplesToProduce);

    generatedTuples += buffer.getNumberOfTuples();
    generatedBuffers++;

    x_TRACE("MemorySource::receiveData filled buffer with tuples= {}", buffer.getNumberOfTuples());
    if (buffer.getNumberOfTuples() == 0) {
        return std::nullopt;
    }
    return buffer;
}

std::string MemorySource::toString() const { return "MemorySource"; }

x::SourceType MemorySource::getType() const { return SourceType::MEMORY_SOURCE; }
}// namespace x