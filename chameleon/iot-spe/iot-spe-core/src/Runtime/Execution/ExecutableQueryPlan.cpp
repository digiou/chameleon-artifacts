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
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/Execution/ExecutableQueryPlan.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/QueryManager.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Sources/DataSource.hpp>
#include <Util/Logger/Logger.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace x::Runtime::Execution {

// https://github.com/IoTSPE/IoTSPE/issues/4172
// The benchmarking framework does not support queries with Java UDFs.
// This code prints timing information to std::out, so we can disable logging.
// It should be removed once better evaluation infrastructure exists.
// Code taken from https://stacktuts.com/how-to-print-current-time-with-milliseconds-using-c-c-11
void printTimeStamp(const std::string& function,
                    const QueryId& queryId,
                    const QuerySubPlanId& querySubPlanId,
                    const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* parts = std::localtime(&now_c);
    std::stringstream str;
    str << function << ": queryId=" << static_cast<uint64_t>(queryId)
        << ", querySubPlanId=" << static_cast<uint64_t>(querySubPlanId) << "; " << message;
    std::cout << std::put_time(parts, "%Y-%m-%d %H:%M:%S.")
              << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000 << " " << str.str()
              << std::endl;
}

ExecutableQueryPlan::ExecutableQueryPlan(QueryId queryId,
                                         QuerySubPlanId querySubPlanId,
                                         std::vector<DataSourcePtr>&& sources,
                                         std::vector<DataSinkPtr>&& sinks,
                                         std::vector<ExecutablePipelinePtr>&& pipelix,
                                         QueryManagerPtr&& queryManager,
                                         BufferManagerPtr&& bufferManager)
    : queryId(queryId), querySubPlanId(querySubPlanId), sources(std::move(sources)), sinks(std::move(sinks)),
      pipelix(std::move(pipelix)), queryManager(std::move(queryManager)), bufferManager(std::move(bufferManager)),
      qepStatus(Execution::ExecutableQueryPlanStatus::Created),
      qepTerminationStatusFuture(qepTerminationStatusPromise.get_future()) {
    // the +1 is the termination token for the query plan itself
    numOfTerminationTokens.store(1 + this->sources.size() + this->pipelix.size() + this->sinks.size());
}

ExecutableQueryPlanPtr ExecutableQueryPlan::create(QueryId queryId,
                                                   QuerySubPlanId querySubPlanId,
                                                   std::vector<DataSourcePtr> sources,
                                                   std::vector<DataSinkPtr> sinks,
                                                   std::vector<ExecutablePipelinePtr> pipelix,
                                                   QueryManagerPtr queryManager,
                                                   BufferManagerPtr bufferManager) {
    return std::make_shared<ExecutableQueryPlan>(queryId,
                                                 querySubPlanId,
                                                 std::move(sources),
                                                 std::move(sinks),
                                                 std::move(pipelix),
                                                 std::move(queryManager),
                                                 std::move(bufferManager));
}

QueryId ExecutableQueryPlan::getQueryId() const { return queryId; }

const std::vector<ExecutablePipelinePtr>& ExecutableQueryPlan::getPipelix() const { return pipelix; }

QuerySubPlanId ExecutableQueryPlan::getQuerySubPlanId() const { return querySubPlanId; }

ExecutableQueryPlan::~ExecutableQueryPlan() {
    x_DEBUG("destroy qep {} {}", queryId, querySubPlanId);
    x_ASSERT(qepStatus.load() == Execution::ExecutableQueryPlanStatus::Created
                   || qepStatus.load() == Execution::ExecutableQueryPlanStatus::Stopped
                   || qepStatus.load() == Execution::ExecutableQueryPlanStatus::ErrorState,
               "QueryPlan is created but not executing " << queryId);
    destroy();
}

std::shared_future<ExecutableQueryPlanResult> ExecutableQueryPlan::getTerminationFuture() {
    return qepTerminationStatusFuture.share();
}

bool ExecutableQueryPlan::fail() {
    bool ret = true;
    auto expected = Execution::ExecutableQueryPlanStatus::Running;
    if (qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::ErrorState)) {
        x_DEBUG("QueryExecutionPlan: fail query={} sublplan={}", queryId, querySubPlanId);
        for (auto& stage : pipelix) {
            if (!stage->fail()) {
                std::stringstream s;
                s << stage;
                std::string stageAsString = s.str();
                x_ERROR("QueryExecutionPlan: fail failed for stage {}", stageAsString);
                ret = false;
            }
        }
        qepTerminationStatusPromise.set_value(ExecutableQueryPlanResult::Fail);
    }

    if (!ret) {
        qepStatus.store(Execution::ExecutableQueryPlanStatus::ErrorState);
    }
    return ret;
}

ExecutableQueryPlanStatus ExecutableQueryPlan::getStatus() { return qepStatus.load(); }

bool ExecutableQueryPlan::setup() {
    printTimeStamp("setup", queryId, querySubPlanId, "");
    x_DEBUG("QueryExecutionPlan: setup queryId={} querySubPlanId={}", queryId, querySubPlanId);
    auto expected = Execution::ExecutableQueryPlanStatus::Created;
    if (qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::Deployed)) {
        for (auto& stage : pipelix) {
            if (!stage->setup(queryManager, bufferManager)) {
                x_ERROR("QueryExecutionPlan: setup failed! {} {}", queryId, querySubPlanId);
                this->stop();
                return false;
            }
        }
    } else {
        x_THROW_RUNTIME_ERROR("QEP expected to be Created but was not");
    }
    return true;
}

bool ExecutableQueryPlan::start(const StateManagerPtr& stateManager) {
    printTimeStamp("start", queryId, querySubPlanId, "");
    x_DEBUG("QueryExecutionPlan: start query={} subplan={}", queryId, querySubPlanId);
    auto expected = Execution::ExecutableQueryPlanStatus::Deployed;
    if (qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::Running)) {
        for (auto& stage : pipelix) {
            x_DEBUG("ExecutableQueryPlan::start qep={} pipe={}", stage->getQuerySubPlanId(), stage->getPipelineId());
            if (!stage->start(stateManager)) {
                x_ERROR("QueryExecutionPlan: start failed! query={} subplan={}", queryId, querySubPlanId);
                this->stop();
                return false;
            }
        }
    } else {
        x_THROW_RUNTIME_ERROR("QEP expected to be Deployed but was not");
    }
    return true;
}

QueryManagerPtr ExecutableQueryPlan::getQueryManager() { return queryManager; }

BufferManagerPtr ExecutableQueryPlan::getBufferManager() { return bufferManager; }

const std::vector<DataSourcePtr>& ExecutableQueryPlan::getSources() const { return sources; }

const std::vector<DataSinkPtr>& ExecutableQueryPlan::getSinks() const { return sinks; }

bool ExecutableQueryPlan::stop() {
    bool allStagesStopped = true;
    auto expected = Execution::ExecutableQueryPlanStatus::Running;
    x_DEBUG("QueryExecutionPlan: stop queryId={} querySubPlanId={}", queryId, querySubPlanId);
    if (qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::Stopped)) {
        x_DEBUG("QueryExecutionPlan: stop {}-{} is marked as stopped now", queryId, querySubPlanId);
        for (auto& stage : pipelix) {
            if (!stage->stop(QueryTerminationType::HardStop)) {
                std::stringstream s;
                s << stage;
                std::string stageAsString = s.str();
                x_ERROR("QueryExecutionPlan: stop failed for stage {}", stageAsString);
                allStagesStopped = false;
            }
        }

        bufferManager.reset();

        if (allStagesStopped) {
            qepTerminationStatusPromise.set_value(ExecutableQueryPlanResult::Ok);
            return true;// correct stop
        }

        qepStatus.store(Execution::ExecutableQueryPlanStatus::ErrorState);
        qepTerminationStatusPromise.set_value(ExecutableQueryPlanResult::Fail);

        return false;// one stage failed to stop
    }

    if (expected == Execution::ExecutableQueryPlanStatus::Stopped) {
        return true;// we have tried to stop the same QEP twice..
    }
    x_ERROR("Something is wrong with query {} as it was not possible to stop", querySubPlanId);
    // if we get there it mean the CAS failed and expected is the current value
    while (!qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::ErrorState)) {
        // try to install ErrorState
    }

    qepTerminationStatusPromise.set_value(ExecutableQueryPlanResult::Fail);

    bufferManager.reset();
    return false;
}

void ExecutableQueryPlan::postReconfigurationCallback(ReconfigurationMessage& task) {
    printTimeStamp("postReconfigurationCallback",
                   queryId,
                   querySubPlanId,
                   getQueryManager()->getQueryStatistics(querySubPlanId)->getQueryStatisticsAsString());
    Reconfigurable::postReconfigurationCallback(task);
    switch (task.getType()) {
        case ReconfigurationType::FailEndOfStream: {
            x_DEBUG("Executing FailEndOfStream on qep queryId={} querySubPlanId={}", queryId, querySubPlanId);
            x_ASSERT2_FMT((numOfTerminationTokens.fetch_sub(1) == 1),
                            "Invalid FailEndOfStream on qep queryId=" << queryId << " querySubPlanId=" << querySubPlanId);
            x_ASSERT2_FMT(fail(), "Cannot fail queryId=" << queryId << " querySubPlanId=" << querySubPlanId);
            x_DEBUG("QueryExecutionPlan: query plan {} subplan {} is marked as failed now", queryId, querySubPlanId);
            queryManager->notifyQueryStatusChange(shared_from_base<ExecutableQueryPlan>(),
                                                  Execution::ExecutableQueryPlanStatus::ErrorState);
            break;
        }
        case ReconfigurationType::HardEndOfStream: {
            x_DEBUG("Executing HardEndOfStream on qep queryId={} querySubPlanId={}", queryId, querySubPlanId);
            x_ASSERT2_FMT((numOfTerminationTokens.fetch_sub(1) == 1),
                            "Invalid HardEndOfStream on qep queryId=" << queryId << " querySubPlanId=" << querySubPlanId);
            x_ASSERT2_FMT(stop(), "Cannot hard stop qep queryId=" << queryId << " querySubPlanId=" << querySubPlanId);
            queryManager->notifyQueryStatusChange(shared_from_base<ExecutableQueryPlan>(),
                                                  Execution::ExecutableQueryPlanStatus::Stopped);
            break;
        }
        case ReconfigurationType::SoftEndOfStream: {
            x_DEBUG("QueryExecutionPlan: soft stop request received for query plan {} sub plan {} left tokens = {}",
                      queryId,
                      querySubPlanId,
                      numOfTerminationTokens);
            if (numOfTerminationTokens.fetch_sub(1) == 1) {
                auto expected = Execution::ExecutableQueryPlanStatus::Running;
                if (qepStatus.compare_exchange_strong(expected, Execution::ExecutableQueryPlanStatus::Finished)) {
                    // if CAS fails - it means the query was already stopped or failed
                    x_DEBUG("QueryExecutionPlan: query plan {} subplan {} is marked as (soft) stopped now",
                              queryId,
                              querySubPlanId);
                    qepTerminationStatusPromise.set_value(ExecutableQueryPlanResult::Ok);
                    queryManager->notifyQueryStatusChange(shared_from_base<ExecutableQueryPlan>(),
                                                          Execution::ExecutableQueryPlanStatus::Finished);
                    return;
                }
            } else {
                x_DEBUG("QueryExecutionPlan: query plan {} subplan {} was already marked as stopped now",
                          queryId,
                          querySubPlanId);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void ExecutableQueryPlan::destroy() {
    // sanity checks: ensure we can destroy stopped instances
    auto expected = 0u;
    if (qepStatus == ExecutableQueryPlanStatus::Created) {
        return;// there is nothing to destroy;
    }
    x_ASSERT2_FMT(numOfTerminationTokens.compare_exchange_strong(expected, uint32_t(-1)),
                    "Destroying still active query plan, tokens left=" << expected);
    for (const auto& source : sources) {
        x_ASSERT(!source->isRunning(), "Source " << source->getOperatorId() << " is still running");
    }
    for (const auto& pipeline : pipelix) {
        x_ASSERT(!pipeline->isRunning(), "Pipeline " << pipeline->getPipelineId() << " is still running");
    }
    sources.clear();
    pipelix.clear();
    sinks.clear();
    bufferManager.reset();
}

void ExecutableQueryPlan::onEvent(BaseEvent&) {
    // nop :: left on purpose -> fill this in when you want to support events
}
void ExecutableQueryPlan::notifySourceCompletion(DataSourcePtr source, QueryTerminationType terminationType) {
    x_DEBUG("QEP {} Source {} is notifying completion: {}", querySubPlanId, source->getOperatorId(), terminationType);
    x_ASSERT2_FMT(qepStatus.load() == ExecutableQueryPlanStatus::Running,
                    "Cannot complete source on non running query plan id=" << querySubPlanId);
    auto it = std::find(sources.begin(), sources.end(), source);
    x_ASSERT2_FMT(it != sources.end(),
                    "Cannot find source " << source->getOperatorId() << " in sub query plan " << querySubPlanId);
    uint32_t tokensLeft = numOfTerminationTokens.fetch_sub(1);
    x_ASSERT2_FMT(tokensLeft >= 1, "Source was last termination token for " << querySubPlanId << " = " << terminationType);
    x_DEBUG("QEP {} Source {} is terminated; tokens left = {}", querySubPlanId, source->getOperatorId(), (tokensLeft - 1));
    // the following check is necessary because a data sources first emits an EoS marker and then calls this method.
    // However, it might happen that the marker is so fast that one possible execution is as follows:
    // 1) Data Source emits EoS marker
    // 2) Next pipeline/sink picks the marker and finishes by decreasing `numOfTerminationTokens`
    // 3) DataSource invokes `notifySourceCompletion`
    // as a result, the data source might be the last one to decrease the `numOfTerminationTokens` thus it has to
    // trigger the QEP reconfiguration
    if (tokensLeft == 2) {// this is the second last token to be removed (last one is the qep itself)
        auto reconfMessageQEP = ReconfigurationMessage(getQueryId(),
                                                       getQuerySubPlanId(),
                                                       terminationType == QueryTerminationType::Graceful
                                                           ? ReconfigurationType::SoftEndOfStream
                                                           : (terminationType == QueryTerminationType::HardStop
                                                                  ? ReconfigurationType::HardEndOfStream
                                                                  : ReconfigurationType::FailEndOfStream),
                                                       Reconfigurable::shared_from_this<ExecutableQueryPlan>());
        queryManager->addReconfigurationMessage(getQueryId(), getQuerySubPlanId(), reconfMessageQEP, false);
    }
}

void ExecutableQueryPlan::notifyPipelineCompletion(ExecutablePipelinePtr pipeline, QueryTerminationType terminationType) {
    x_ASSERT2_FMT(qepStatus.load() == ExecutableQueryPlanStatus::Running,
                    "Cannot complete pipeline on non running query plan id=" << querySubPlanId);
    auto it = std::find(pipelix.begin(), pipelix.end(), pipeline);
    x_ASSERT2_FMT(it != pipelix.end(),
                    "Cannot find pipeline " << pipeline->getPipelineId() << " in sub query plan " << querySubPlanId);
    uint32_t tokensLeft = numOfTerminationTokens.fetch_sub(1);
    x_ASSERT2_FMT(tokensLeft >= 1, "Pipeline was last termination token for " << querySubPlanId);
    x_DEBUG("QEP {} Pipeline {} is terminated; tokens left = {}", querySubPlanId, pipeline->getPipelineId(), (tokensLeft - 1));
    // the same applies here for the pipeline
    if (tokensLeft == 2) {// this is the second last token to be removed (last one is the qep itself)
        auto reconfMessageQEP = ReconfigurationMessage(getQueryId(),
                                                       getQuerySubPlanId(),
                                                       terminationType == QueryTerminationType::Graceful
                                                           ? ReconfigurationType::SoftEndOfStream
                                                           : (terminationType == QueryTerminationType::HardStop
                                                                  ? ReconfigurationType::HardEndOfStream
                                                                  : ReconfigurationType::FailEndOfStream),
                                                       Reconfigurable::shared_from_this<ExecutableQueryPlan>());
        queryManager->addReconfigurationMessage(getQueryId(), getQuerySubPlanId(), reconfMessageQEP, false);
    }
}

void ExecutableQueryPlan::notifySinkCompletion(DataSinkPtr sink, QueryTerminationType terminationType) {
    x_ASSERT2_FMT(qepStatus.load() == ExecutableQueryPlanStatus::Running,
                    "Cannot complete sink on non running query plan id=" << querySubPlanId);
    auto it = std::find(sinks.begin(), sinks.end(), sink);
    x_ASSERT2_FMT(it != sinks.end(), "Cannot find sink " << sink->toString() << " in sub query plan " << querySubPlanId);
    uint32_t tokensLeft = numOfTerminationTokens.fetch_sub(1);
    x_ASSERT2_FMT(tokensLeft >= 1, "Sink was last termination token for " << querySubPlanId);
    x_DEBUG("QEP {} Sink {} is terminated; tokens left = {}", querySubPlanId, sink->toString(), (tokensLeft - 1));
    // sinks also require to spawn a reconfig task for the qep
    if (tokensLeft == 2) {// this is the second last token to be removed (last one is the qep itself)
        auto reconfMessageQEP = ReconfigurationMessage(getQueryId(),
                                                       getQuerySubPlanId(),
                                                       terminationType == QueryTerminationType::Graceful
                                                           ? ReconfigurationType::SoftEndOfStream
                                                           : (terminationType == QueryTerminationType::HardStop
                                                                  ? ReconfigurationType::HardEndOfStream
                                                                  : ReconfigurationType::FailEndOfStream),
                                                       Reconfigurable::shared_from_this<ExecutableQueryPlan>());
        queryManager->addReconfigurationMessage(getQueryId(), getQuerySubPlanId(), reconfMessageQEP, false);
    }
}

}// namespace x::Runtime::Execution
