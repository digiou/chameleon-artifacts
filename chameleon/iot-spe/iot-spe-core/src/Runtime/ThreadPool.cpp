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
#include "Runtime/QueryManager.hpp"
#include <Exceptions/TaskExecutionException.hpp>
#include <Network/NetworkChannel.hpp>
#include <Runtime/xThread.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/Task.hpp>
#include <Runtime/ThreadPool.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadBarrier.hpp>
#include <Util/ThreadNaming.hpp>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <utility>

#ifdef __linux__
#include <Runtime/HardwareManager.hpp>
#endif

#ifdef ENABLE_PAPI_PROFILER
#include <Runtime/Profiler/PAPIProfiler.hpp>
#endif
namespace x::Runtime {

ThreadPool::ThreadPool(uint64_t nodeId,
                       QueryManagerPtr queryManager,
                       uint32_t numThreads,
                       std::vector<BufferManagerPtr> bufferManagers,
                       uint64_t numberOfBuffersPerWorker,
                       HardwareManagerPtr hardwareManager,
                       std::vector<uint64_t> workerPinningPositionList)
    : nodeId(nodeId), numThreads(numThreads), queryManager(std::move(queryManager)), bufferManagers(bufferManagers),
      numberOfBuffersPerWorker(numberOfBuffersPerWorker), workerPinningPositionList(workerPinningPositionList),
      hardwareManager(hardwareManager) {}

ThreadPool::~ThreadPool() {
    x_DEBUG("Threadpool: Destroying Thread Pool");
    stop();
    x_DEBUG("QueryManager: Destroy threads Queue");
    threads.clear();
}

void ThreadPool::runningRoutine(WorkerContext&& workerContext) {
    while (running) {
        try {
            switch (queryManager->processNextTask(running, workerContext)) {
                case ExecutionResult::Finished:
                case ExecutionResult::Ok: {
                    break;
                }
                case ExecutionResult::AllFinished: {
                    x_DEBUG("Threadpool got poison pill - shutting down...");
                    running = false;
                    break;
                }
                case ExecutionResult::Error: {
                    // TODO add here error handling (see issues 524 and 463)
                    x_ERROR("Threadpool: finished task with error");
                    running = false;
                    break;
                }
                default: {
                    x_ASSERT(false, "unsupported");
                }
            }
        } catch (TaskExecutionException const& taskException) {
            x_ERROR("Got fatal error on thread {}: {}", workerContext.getId(), taskException.what());
            queryManager->notifyTaskFailure(taskException.getExecutable(), std::string(taskException.what()));
        }
    }
    // to drain the queue for pending reconfigurations
    // after this no need to care for error handling
    try {
        queryManager->processNextTask(running, workerContext);
    } catch (std::exception const& error) {
        x_ERROR("Got fatal error on thread {}: {}", workerContext.getId(), error.what());
        x_THROW_RUNTIME_ERROR("Got fatal error on thread " << workerContext.getId() << ": " << error.what());
    }
    x_DEBUG("Threadpool: end runningRoutine");
}

bool ThreadPool::start(const std::vector<uint64_t> threadToQueueMapping) {
    auto barrier = std::make_shared<ThreadBarrier>(numThreads + 1);
    std::unique_lock lock(reconfigLock);
    if (running) {
        x_DEBUG("Threadpool:start already running, return false");
        return false;
    }
    running = true;

    /* spawn threads */
    x_DEBUG("Threadpool: Spawning {} threads", numThreads);
    for (uint64_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, barrier, &threadToQueueMapping]() {
            setThreadName("Wrk-%d-%d", nodeId, i);
            BufferManagerPtr localBufferManager;
            uint64_t queueIdx = threadToQueueMapping.size() ? threadToQueueMapping[i] : 0;
#if defined(__linux__)
            if (workerPinningPositionList.size() != 0) {
                x_ASSERT(numThreads <= workerPinningPositionList.size(),
                           "Not enough worker positions for pinning are provided");
                uint64_t maxPosition = *std::max_element(workerPinningPositionList.begin(), workerPinningPositionList.end());
                x_ASSERT(maxPosition < std::thread::hardware_concurrency(),
                           "pinning position thread is out of cpu range maxPosition=" << maxPosition);
                //pin core
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(workerPinningPositionList[i], &cpuset);
                int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    x_ERROR("Error calling pthread_setaffinity_np: {}", rc);
                } else {
                    x_WARNING("worker {} pins to core={}", i, workerPinningPositionList[i]);
                }
            }
#endif
            localBufferManager = bufferManagers[queueIdx];

            barrier->wait();
            x_ASSERT(localBufferManager, "localBufferManager is null");
#ifdef ENABLE_PAPI_PROFILER
            auto path = std::filesystem::path("worker_" + std::to_string(xThread::getId()) + ".csv");
            auto profiler = std::make_shared<Profiler::PapiCpuProfiler>(Profiler::PapiCpuProfiler::Presets::CachePresets,
                                                                        std::ofstream(path, std::ofstream::out),
                                                                        xThread::getId(),
                                                                        xThread::getId());
            queryManager->cpuProfilers[xThread::getId() % queryManager->cpuProfilers.size()] = profiler;
#endif
            // TODO (2310) properly initialize the profiler with a file, thread, and core id
            auto workerId = xThread::getId();
            x_DEBUG("worker {} with workerId {} pins to queue {}", i, workerId, queueIdx);
            runningRoutine(WorkerContext(workerId, localBufferManager, numberOfBuffersPerWorker, queueIdx));
        });
    }
    barrier->wait();
    x_DEBUG("Threadpool: start return from start");
    return true;
}

bool ThreadPool::stop() {
    std::unique_lock lock(reconfigLock);
    x_DEBUG("ThreadPool: stop thread pool while {} with {} threads", (running.load() ? "running" : "not running"), numThreads);
    auto expected = true;
    if (!running.compare_exchange_strong(expected, false)) {
        return false;
    }
    /* wake up all threads in the query manager,
 * so they notice the change in the run variable */
    x_DEBUG("Threadpool: Going to unblock {} threads", numThreads);
    queryManager->poisonWorkers();
    /* join all threads if possible */
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();
    x_DEBUG("Threadpool: stop finished");
    return true;
}

uint32_t ThreadPool::getNumberOfThreads() const {
    std::unique_lock lock(reconfigLock);
    return numThreads;
}

}// namespace x::Runtime