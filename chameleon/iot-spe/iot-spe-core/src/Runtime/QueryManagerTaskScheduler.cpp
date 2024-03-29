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
#include <Network/NetworkSink.hpp>
#include <Network/NetworkSource.hpp>
#include <Runtime/AsyncTaskExecutor.hpp>
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/ExecutableQueryPlan.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/HardwareManager.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/ThreadPool.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Util/Core.hpp>
#include <Util/Logger//Logger.hpp>
#include <iostream>
#include <memory>
#include <stack>
#include <utility>

namespace x::Runtime {

namespace detail {

class ReconfigurationPipelineExecutionContext : public Execution::PipelineExecutionContext {
  public:
    explicit ReconfigurationPipelineExecutionContext(QuerySubPlanId queryExecutionPlanId, QueryManagerPtr queryManager)
        : Execution::PipelineExecutionContext(
            -1,// this is a dummy pipelineID
            queryExecutionPlanId,
            queryManager->getBufferManager(),
            queryManager->getNumberOfWorkerThreads(),
            [](TupleBuffer&, x::Runtime::WorkerContext&) {
            },
            [](TupleBuffer&) {
            },
            std::vector<Execution::OperatorHandlerPtr>()) {
        // nop
    }
};

class ReconfigurationEntryPointPipelixtage : public Execution::ExecutablePipelixtage {
    using base = Execution::ExecutablePipelixtage;

  public:
    explicit ReconfigurationEntryPointPipelixtage() : base(PipelixtageArity::Unary) {
        // nop
    }

    ExecutionResult execute(TupleBuffer& buffer, Execution::PipelineExecutionContext&, WorkerContextRef workerContext) {
        x_TRACE(
            "QueryManager: AbstractQueryManager::addReconfigurationMessage ReconfigurationMessageEntryPoint begin on thread {}",
            workerContext.getId());
        auto* task = buffer.getBuffer<ReconfigurationMessage>();
        x_TRACE("QueryManager: AbstractQueryManager::addReconfigurationMessage ReconfigurationMessageEntryPoint going to wait "
                  "on thread {}",
                  workerContext.getId());
        task->wait();
        x_TRACE("QueryManager: AbstractQueryManager::addReconfigurationMessage ReconfigurationMessageEntryPoint going to "
                  "reconfigure on thread {}",
                  workerContext.getId());
        task->getInstance()->reconfigure(*task, workerContext);
        x_TRACE("QueryManager: AbstractQueryManager::addReconfigurationMessage ReconfigurationMessageEntryPoint post callback "
                  "on thread {}",
                  workerContext.getId());
        task->postReconfiguration();
        x_TRACE("QueryManager: AbstractQueryManager::addReconfigurationMessage ReconfigurationMessageEntryPoint completed on "
                  "thread {}",
                  workerContext.getId());
        task->postWait();
        return ExecutionResult::Ok;
    }
};
}// namespace detail

ExecutionResult DynamicQueryManager::processNextTask(bool running, WorkerContext& workerContext) {
    x_TRACE("QueryManager: AbstractQueryManager::getWork wait get lock");
    Task task;
    if (running) {
        taskQueue.blockingRead(task);

#ifdef ENABLE_PAPI_PROFILER
        auto profiler = cpuProfilers[xThread::getId() % cpuProfilers.size()];
        auto numOfInputTuples = task.getNumberOfInputTuples();
        profiler->startSampling();
#endif

        x_TRACE("QueryManager: provide task {} to thread (getWork())", task.toString());
        ExecutionResult result = task(workerContext);
#ifdef ENABLE_PAPI_PROFILER
        profiler->stopSampling(numOfInputTuples);
#endif

        switch (result) {
            //OK comes from sinks and intermediate operators
            case ExecutionResult::Ok: {
                completedWork(task, workerContext);
                return ExecutionResult::Ok;
            }
            //Finished indicate that the processing is done
            case ExecutionResult::Finished: {
                completedWork(task, workerContext);
                return ExecutionResult::Finished;
            }
            default: {
                return result;
            }
        }
    } else {
        return terminateLoop(workerContext);
    }
}

ExecutionResult MultiQueueQueryManager::processNextTask(bool running, WorkerContext& workerContext) {
    x_TRACE("QueryManager: AbstractQueryManager::getWork wait get lock");
    Task task;
    if (running) {
        taskQueues[workerContext.getQueueId()].blockingRead(task);

#ifdef ENABLE_PAPI_PROFILER
        auto profiler = cpuProfilers[xThread::getId() % cpuProfilers.size()];
        auto numOfInputTuples = task.getNumberOfInputTuples();
        profiler->startSampling();
#endif

        x_TRACE("QueryManager: provide task {} to thread (getWork())", task.toString());
        auto result = task(workerContext);
#ifdef ENABLE_PAPI_PROFILER
        profiler->stopSampling(numOfInputTuples);
#endif

        switch (result) {
            case ExecutionResult::Ok: {
                completedWork(task, workerContext);
                return ExecutionResult::Ok;
            }
            default: {
                return result;
            }
        }
    } else {
        return terminateLoop(workerContext);
    }
}
ExecutionResult DynamicQueryManager::terminateLoop(WorkerContext& workerContext) {
    bool hitReconfiguration = false;
    Task task;
    while (taskQueue.read(task)) {
        if (!hitReconfiguration) {// execute all pending tasks until first reconfiguration
            task(workerContext);
            if (task.isReconfiguration()) {
                hitReconfiguration = true;
            }
        } else {
            if (task.isReconfiguration()) {// execute only pending reconfigurations
                task(workerContext);
            }
        }
    }
    return ExecutionResult::Finished;
}

void DynamicQueryManager::addWorkForNextPipeline(TupleBuffer& buffer,
                                                 Execution::SuccessorExecutablePipeline executable,
                                                 uint32_t queueId) {
    x_TRACE("Add Work for executable for queue={}", queueId);
    if (auto nextPipeline = std::get_if<Execution::ExecutablePipelinePtr>(&executable); nextPipeline) {
        if (!(*nextPipeline)->isRunning()) {
            // we ignore task if the pipeline is not running anymore.
            x_WARNING("Pushed task for non running executable pipeline id={}", (*nextPipeline)->getPipelineId());
            return;
        }

        taskQueue.blockingWrite(Task(executable, buffer, getNextTaskId()));

    } else if (auto sink = std::get_if<DataSinkPtr>(&executable); sink) {
        std::stringstream s;
        s << buffer;
        std::string bufferString = s.str();
        x_TRACE("QueryManager: added Task for Sink {} inputBuffer {} queueId={}",
                  sink->get()->toString(),
                  bufferString,
                  queueId);

        taskQueue.blockingWrite(Task(executable, buffer, getNextTaskId()));
    } else {
        x_THROW_RUNTIME_ERROR("This should not happen");
    }
}

ExecutionResult MultiQueueQueryManager::terminateLoop(WorkerContext& workerContext) {
    bool hitReconfiguration = false;
    Task task;
    while (taskQueues[workerContext.getQueueId()].read(task)) {
        if (!hitReconfiguration) {// execute all pending tasks until first reconfiguration
            task(workerContext);
            if (task.isReconfiguration()) {
                hitReconfiguration = true;
            }
        } else {
            if (task.isReconfiguration()) {// execute only pending reconfigurations
                task(workerContext);
            }
        }
    }
    return ExecutionResult::Finished;
}

void MultiQueueQueryManager::addWorkForNextPipeline(TupleBuffer& buffer,
                                                    Execution::SuccessorExecutablePipeline executable,
                                                    uint32_t queueId) {
    x_TRACE("Add Work for executable for queue={}", queueId);
    x_ASSERT(queueId < taskQueues.size(), "Invalid queue id");
    if (auto nextPipeline = std::get_if<Execution::ExecutablePipelinePtr>(&executable)) {
        if (!(*nextPipeline)->isRunning()) {
            // we ignore task if the pipeline is not running anymore.
            x_WARNING("Pushed task for non running executable pipeline id={}", (*nextPipeline)->getPipelineId());
            return;
        }
        std::stringstream s;
        s << buffer;
        std::string bufferString = s.str();
        x_TRACE("QueryManager: added Task this pipelineID={} for Number of next pipelix {} inputBuffer {} queueId={}",
                  (*nextPipeline)->getPipelineId(),
                  (*nextPipeline)->getSuccessors().size(),
                  bufferString,
                  queueId);

        taskQueues[queueId].write(Task(executable, buffer, getNextTaskId()));
    } else if (auto sink = std::get_if<DataSinkPtr>(&executable)) {
        std::stringstream s;
        s << buffer;
        std::string bufferString = s.str();
        x_TRACE("QueryManager: added Task for Sink {} inputBuffer {} queueId={}",
                  sink->get()->toString(),
                  bufferString,
                  queueId);

        taskQueues[queueId].write(Task(executable, buffer, getNextTaskId()));
    } else {
        x_THROW_RUNTIME_ERROR("This should not happen");
    }
}

void DynamicQueryManager::updateStatistics(const Task& task,
                                           QueryId queryId,
                                           QuerySubPlanId querySubPlanId,
                                           PipelineId pipelineId,
                                           WorkerContext& workerContext) {
    AbstractQueryManager::updateStatistics(task, queryId, querySubPlanId, pipelineId, workerContext);
#ifndef LIGHT_WEIGHT_STATISTICS
    if (queryToStatisticsMap.contains(querySubPlanId)) {
        auto statistics = queryToStatisticsMap.find(querySubPlanId);
        // with multiple queryIdAndCatalogEntryMapping this wont be correct
        auto qSize = taskQueue.size();
        statistics->incQueueSizeSum(qSize > 0 ? qSize : 0);
    }
#endif
}

void MultiQueueQueryManager::updateStatistics(const Task& task,
                                              QueryId queryId,
                                              QuerySubPlanId querySubPlanId,
                                              PipelineId pipelineId,
                                              WorkerContext& workerContext) {
    AbstractQueryManager::updateStatistics(task, queryId, querySubPlanId, pipelineId, workerContext);
#ifndef LIGHT_WEIGHT_STATISTICS
    if (queryToStatisticsMap.contains(querySubPlanId)) {
        auto statistics = queryToStatisticsMap.find(querySubPlanId);
        auto qSize = taskQueues[workerContext.getQueueId()].size();
        statistics->incQueueSizeSum(qSize > 0 ? qSize : 0);
    }
#endif
}

void AbstractQueryManager::updateStatistics(const Task& task,
                                            QueryId queryId,
                                            QuerySubPlanId querySubPlanId,
                                            PipelineId pipelineId,
                                            WorkerContext& workerContext) {
    tempCounterTasksCompleted[workerContext.getId() % tempCounterTasksCompleted.size()].fetch_add(1);
#ifndef LIGHT_WEIGHT_STATISTICS
    if (queryToStatisticsMap.contains(querySubPlanId)) {
        auto statistics = queryToStatisticsMap.find(querySubPlanId);

        auto now =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();

        statistics->setTimestampFirstProcessedTask(now, true);
        statistics->setTimestampLastProcessedTask(now);
        statistics->incProcessedTasks();
        statistics->incProcessedBuffers();
        auto creation = task.getBufferRef().getCreationTimestampInMS();
        x_ASSERT((unsigned long) now >= creation, "timestamp is in the past");
        statistics->incLatencySum(now - creation);

        for (auto& bufferManager : bufferManagers) {
            statistics->incAvailableGlobalBufferSum(bufferManager->getAvailableBuffers());
            statistics->incAvailableFixedBufferSum(bufferManager->getAvailableBuffersInFixedSizePools());
        }

        statistics->incTasksPerPipelineId(pipelineId, workerContext.getId());

#ifdef x_BENCHMARKS_DETAILED_LATENCY_MEASUREMENT
        statistics->addTimestampToLatencyValue(now, diff);
#endif
        statistics->incProcessedTuple(task.getNumberOfInputTuples());
    } else {
        using namespace std::string_literals;

        x_ERROR("queryToStatisticsMap not set for {} this should only happen for testing", std::to_string(queryId));
    }
#endif
}

void AbstractQueryManager::completedWork(Task& task, WorkerContext& wtx) {
    x_TRACE("AbstractQueryManager::completedWork: Work for task={} worker ctx id={}", task.toString(), wtx.getId());
    if (task.isReconfiguration()) {
        return;
    }

    QuerySubPlanId querySubPlanId = -1;
    QueryId queryId = -1;
    PipelineId pipelineId = -1;
    auto executable = task.getExecutable();
    if (auto* sink = std::get_if<DataSinkPtr>(&executable)) {
        querySubPlanId = (*sink)->getParentPlanId();
        queryId = (*sink)->getQueryId();
        x_TRACE("AbstractQueryManager::completedWork: task for sink querySubPlanId={}", querySubPlanId);
    } else if (auto* executablePipeline = std::get_if<Execution::ExecutablePipelinePtr>(&executable)) {
        querySubPlanId = (*executablePipeline)->getQuerySubPlanId();
        queryId = (*executablePipeline)->getQueryId();
        pipelineId = (*executablePipeline)->getPipelineId();
        x_TRACE("AbstractQueryManager::completedWork: task for exec pipeline isreconfig={}",
                  (*executablePipeline)->isReconfiguration());
    }
    updateStatistics(task, queryId, querySubPlanId, pipelineId, wtx);
}

bool MultiQueueQueryManager::addReconfigurationMessage(QueryId queryId,
                                                       QuerySubPlanId queryExecutionPlanId,
                                                       const ReconfigurationMessage& message,
                                                       bool blocking) {
    x_DEBUG("QueryManager: AbstractQueryManager::addReconfigurationMessage begins on plan {} blocking={} type {}",
              queryExecutionPlanId,
              blocking,
              magic_enum::enum_name(message.getType()));
    x_ASSERT2_FMT(threadPool->isRunning(), "thread pool not running");
    auto optBuffer = bufferManagers[0]->getUnpooledBuffer(sizeof(ReconfigurationMessage));
    x_ASSERT(optBuffer, "invalid buffer");
    auto buffer = optBuffer.value();
    new (buffer.getBuffer()) ReconfigurationMessage(message, numberOfThreadsPerQueue, blocking);
    return addReconfigurationMessage(queryId, queryExecutionPlanId, std::move(buffer), blocking);
}

bool DynamicQueryManager::addReconfigurationMessage(QueryId queryId,
                                                    QuerySubPlanId queryExecutionPlanId,
                                                    const ReconfigurationMessage& message,
                                                    bool blocking) {
    x_DEBUG("QueryManager: AbstractQueryManager::addReconfigurationMessage begins on plan {} blocking={} type {}",
              queryExecutionPlanId,
              blocking,
              magic_enum::enum_name(message.getType()));
    x_ASSERT2_FMT(threadPool->isRunning(), "thread pool not running");
    auto optBuffer = bufferManagers[0]->getUnpooledBuffer(sizeof(ReconfigurationMessage));
    x_ASSERT(optBuffer, "invalid buffer");
    auto buffer = optBuffer.value();
    new (buffer.getBuffer()) ReconfigurationMessage(message, threadPool->getNumberOfThreads(), blocking);// memcpy using copy ctor
    return addReconfigurationMessage(queryId, queryExecutionPlanId, std::move(buffer), blocking);
}

bool DynamicQueryManager::addReconfigurationMessage(QueryId queryId,
                                                    QuerySubPlanId queryExecutionPlanId,
                                                    TupleBuffer&& buffer,
                                                    bool blocking) {
    std::unique_lock reconfLock(reconfigurationMutex);
    auto* task = buffer.getBuffer<ReconfigurationMessage>();
    x_DEBUG("QueryManager: AbstractQueryManager::addReconfigurationMessage begins on plan {} blocking={} type {}",
              queryExecutionPlanId,
              blocking,
              magic_enum::enum_name(task->getType()));
    x_ASSERT2_FMT(threadPool->isRunning(), "thread pool not running");
    auto pipelineContext =
        std::make_shared<detail::ReconfigurationPipelineExecutionContext>(queryExecutionPlanId, inherited0::shared_from_this());
    auto reconfigurationExecutable = std::make_shared<detail::ReconfigurationEntryPointPipelixtage>();
    auto pipeline = Execution::ExecutablePipeline::create(-1,
                                                          queryId,
                                                          queryExecutionPlanId,
                                                          inherited0::shared_from_this(),
                                                          pipelineContext,
                                                          reconfigurationExecutable,
                                                          1,
                                                          std::vector<Execution::SuccessorExecutablePipeline>(),
                                                          true);

    for (uint64_t threadId = 0; threadId < threadPool->getNumberOfThreads(); threadId++) {
        taskQueue.blockingWrite(Task(pipeline, buffer, getNextTaskId()));
    }

    reconfLock.unlock();
    if (blocking) {
        task->postWait();
        task->postReconfiguration();
    }
    //    }
    return true;
}

bool MultiQueueQueryManager::addReconfigurationMessage(QueryId queryId,
                                                       QuerySubPlanId queryExecutionPlanId,
                                                       TupleBuffer&& buffer,
                                                       bool blocking) {
    std::unique_lock reconfLock(reconfigurationMutex);
    auto* task = buffer.getBuffer<ReconfigurationMessage>();
    x_DEBUG("QueryManager: AbstractQueryManager::addReconfigurationMessage begins on plan {} blocking={} type {} to queue={}",
              queryExecutionPlanId,
              blocking,
              magic_enum::enum_name(task->getType()),
              queryToTaskQueueIdMap[queryId]);
    x_ASSERT2_FMT(threadPool->isRunning(), "thread pool not running");
    auto pipelineContext =
        std::make_shared<detail::ReconfigurationPipelineExecutionContext>(queryExecutionPlanId, inherited0::shared_from_this());
    auto reconfigurationExecutable = std::make_shared<detail::ReconfigurationEntryPointPipelixtage>();
    auto pipeline = Execution::ExecutablePipeline::create(-1,
                                                          queryId,
                                                          queryExecutionPlanId,
                                                          inherited0::shared_from_this(),
                                                          pipelineContext,
                                                          reconfigurationExecutable,
                                                          1,
                                                          std::vector<Execution::SuccessorExecutablePipeline>(),
                                                          true);

    for (uint64_t threadId = 0; threadId < numberOfThreadsPerQueue; threadId++) {
        taskQueues[queryToTaskQueueIdMap[queryId]].blockingWrite(Task(pipeline, buffer, getNextTaskId()));
    }

    reconfLock.unlock();
    if (blocking) {
        task->postWait();
        task->postReconfiguration();
    }
    //    }
    return true;
}

namespace detail {
class PoisonPillEntryPointPipelixtage : public Execution::ExecutablePipelixtage {
    using base = Execution::ExecutablePipelixtage;

  public:
    explicit PoisonPillEntryPointPipelixtage() : base(PipelixtageArity::Unary) {
        // nop
    }

    virtual ~PoisonPillEntryPointPipelixtage() = default;

    ExecutionResult execute(TupleBuffer&, Execution::PipelineExecutionContext&, WorkerContextRef) {
        return ExecutionResult::AllFinished;
    }
};
}// namespace detail

void DynamicQueryManager::poisonWorkers() {
    auto optBuffer = bufferManagers[0]->getUnpooledBuffer(1);// there is always one buffer manager
    x_ASSERT(optBuffer, "invalid buffer");
    auto buffer = optBuffer.value();

    auto pipelineContext = std::make_shared<detail::ReconfigurationPipelineExecutionContext>(-1, inherited0::shared_from_this());
    auto pipeline = Execution::ExecutablePipeline::create(-1,// any query plan
                                                          -1,// any sub query plan
                                                          -1,
                                                          inherited0::shared_from_this(),
                                                          pipelineContext,
                                                          std::make_shared<detail::PoisonPillEntryPointPipelixtage>(),
                                                          1,
                                                          std::vector<Execution::SuccessorExecutablePipeline>(),
                                                          true);
    for (auto u{0ul}; u < threadPool->getNumberOfThreads(); ++u) {
        x_DEBUG("Add poison for queue= {}", u);
        taskQueue.blockingWrite(Task(pipeline, buffer, getNextTaskId()));
    }
}

void MultiQueueQueryManager::poisonWorkers() {
    auto optBuffer = bufferManagers[0]->getUnpooledBuffer(1);// there is always one buffer manager
    x_ASSERT(optBuffer, "invalid buffer");
    auto buffer = optBuffer.value();

    auto pipelineContext = std::make_shared<detail::ReconfigurationPipelineExecutionContext>(-1, inherited0::shared_from_this());
    auto pipeline = Execution::ExecutablePipeline::create(-1,// any query plan
                                                          -1,// any sub query plan
                                                          -1,
                                                          inherited0::shared_from_this(),
                                                          pipelineContext,
                                                          std::make_shared<detail::PoisonPillEntryPointPipelixtage>(),
                                                          1,
                                                          std::vector<Execution::SuccessorExecutablePipeline>(),
                                                          true);

    for (auto u{0ul}; u < taskQueues.size(); ++u) {
        for (auto i{0ul}; i < numberOfThreadsPerQueue; ++i) {
            x_DEBUG("Add poision for queue= {}  and thread= {}", u, i);
            taskQueues[u].blockingWrite(Task(pipeline, buffer, getNextTaskId()));
        }
    }
}

}// namespace x::Runtime