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

#include <Runtime/Events.hpp>
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/Execution/ExecutablePipelixtage.hpp>
#include <Runtime/Execution/OperatorHandler.hpp>
#include <Runtime/Execution/PipelineExecutionContext.hpp>
#include <Runtime/QueryManager.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Runtime/WorkerContext.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Util/Logger/Logger.hpp>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;
namespace x::Runtime::Execution {
ExecutablePipeline::ExecutablePipeline(uint64_t pipelineId,
                                       QueryId queryId,
                                       QuerySubPlanId querySubPlanId,
                                       QueryManagerPtr queryManager,
                                       PipelineExecutionContextPtr pipelineExecutionContext,
                                       ExecutablePipelixtagePtr executablePipelixtage,
                                       uint32_t numOfProducingPipelix,
                                       std::vector<SuccessorExecutablePipeline> successorPipelix,
                                       bool reconfiguration)
    : pipelineId(pipelineId), queryId(queryId), querySubPlanId(querySubPlanId), queryManager(queryManager),
      executablePipelixtage(std::move(executablePipelixtage)), pipelineContext(std::move(pipelineExecutionContext)),
      reconfiguration(reconfiguration),
      pipelixtatus(reconfiguration ? Pipelixtatus::PipelineRunning : Pipelixtatus::PipelineCreated),
      activeProducers(numOfProducingPipelix), successorPipelix(std::move(successorPipelix)) {
    // nop
    x_ASSERT(this->executablePipelixtage && this->pipelineContext && numOfProducingPipelix > 0,
               "Wrong pipeline stage argument");
}

ExecutionResult ExecutablePipeline::execute(TupleBuffer& inputBuffer, WorkerContextRef workerContext) {
    x_TRACE("Execute Pipeline Stage with id={} originId={} stage={}", querySubPlanId, inputBuffer.getOriginId(), pipelineId);

    switch (this->pipelixtatus.load()) {
        case Pipelixtatus::PipelineRunning: {
            auto res = executablePipelixtage->execute(inputBuffer, *pipelineContext.get(), workerContext);
            return res;
        }
        case Pipelixtatus::Pipelixtopped: {
            return ExecutionResult::Finished;
        }
        default: {
            x_ERROR("Cannot execute Pipeline Stage with id={} originId={} stage={} as pipeline is not running anymore.",
                      querySubPlanId,
                      inputBuffer.getOriginId(),
                      pipelineId);
            return ExecutionResult::Error;
        }
    }
}

bool ExecutablePipeline::setup(const QueryManagerPtr&, const BufferManagerPtr&) {
    return executablePipelixtage->setup(*pipelineContext.get()) == 0;
}

bool ExecutablePipeline::start(const StateManagerPtr& stateManager) {
    auto expected = Pipelixtatus::PipelineCreated;
    uint32_t localStateVariableId = 0;
    if (pipelixtatus.compare_exchange_strong(expected, Pipelixtatus::PipelineRunning)) {
        auto newReconf = ReconfigurationMessage(queryId,
                                                querySubPlanId,
                                                ReconfigurationType::Initialize,
                                                inherited0::shared_from_this(),
                                                std::make_any<uint32_t>(activeProducers.load()));
        for (const auto& operatorHandler : pipelineContext->getOperatorHandlers()) {
            operatorHandler->start(pipelineContext, stateManager, localStateVariableId);
            localStateVariableId++;
        }
        queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, true);
        executablePipelixtage->start(*pipelineContext.get());
        return true;
    }
    return false;
}

bool ExecutablePipeline::stop(QueryTerminationType) {
    auto expected = Pipelixtatus::PipelineRunning;
    if (pipelixtatus.compare_exchange_strong(expected, Pipelixtatus::Pipelixtopped)) {
        return executablePipelixtage->stop(*pipelineContext.get()) == 0;
    }
    return expected == Pipelixtatus::Pipelixtopped;
}

bool ExecutablePipeline::fail() {
    auto expected = Pipelixtatus::PipelineRunning;
    if (pipelixtatus.compare_exchange_strong(expected, Pipelixtatus::PipelineFailed)) {
        return executablePipelixtage->stop(*pipelineContext.get()) == 0;
    }
    return expected == Pipelixtatus::PipelineFailed;
}

bool ExecutablePipeline::isRunning() const { return pipelixtatus.load() == Pipelixtatus::PipelineRunning; }

const std::vector<SuccessorExecutablePipeline>& ExecutablePipeline::getSuccessors() const { return successorPipelix; }

void ExecutablePipeline::onEvent(Runtime::BaseEvent& event) {
    x_DEBUG("ExecutablePipeline::onEvent(event) called. pipelineId:  {}", this->pipelineId);
    if (event.getEventType() == EventType::kStartSourceEvent) {
        x_DEBUG("ExecutablePipeline: Propagate startSourceEvent further upstream to predecessors, without workerContext.");

        for (auto predecessor : this->pipelineContext->getPredecessors()) {
            if (const auto* sourcePredecessor = std::get_if<std::weak_ptr<x::DataSource>>(&predecessor)) {
                x_DEBUG(
                    "ExecutablePipeline: Found Source in predecessor. Start it with startSourceEvent, without workerContext.");
                sourcePredecessor->lock()->onEvent(event);
            } else if (const auto* pipelinePredecessor =
                           std::get_if<std::weak_ptr<x::Runtime::Execution::ExecutablePipeline>>(&predecessor)) {
                x_DEBUG("ExecutablePipeline: Found Pipeline in Predecessors. Propagate startSourceEvent to it, without "
                          "workerContext.");
                pipelinePredecessor->lock()->onEvent(event);
            }
        }
    }
}

void ExecutablePipeline::onEvent(Runtime::BaseEvent& event, WorkerContextRef workerContext) {
    x_DEBUG("ExecutablePipeline::onEvent(event, wrkContext) called. pipelineId:  {}", this->pipelineId);
    if (event.getEventType() == EventType::kStartSourceEvent) {
        x_DEBUG("ExecutablePipeline: Propagate startSourceEvent further upstream to predecessors, with workerContext.");

        for (auto predecessor : this->pipelineContext->getPredecessors()) {
            if (const auto* sourcePredecessor = std::get_if<std::weak_ptr<x::DataSource>>(&predecessor)) {
                x_DEBUG("ExecutablePipeline: Found Source in predecessor. Start it with startSourceEvent, with workerContext.");
                sourcePredecessor->lock()->onEvent(event, workerContext);
            } else if (const auto* pipelinePredecessor =
                           std::get_if<std::weak_ptr<x::Runtime::Execution::ExecutablePipeline>>(&predecessor)) {
                x_DEBUG(
                    "ExecutablePipeline: Found Pipeline in Predecessors. Propagate startSourceEvent to it, with workerContext.");
                pipelinePredecessor->lock()->onEvent(event, workerContext);
            }
        }
    }
}

uint64_t ExecutablePipeline::getPipelineId() const { return pipelineId; }

QueryId ExecutablePipeline::getQueryId() const { return queryId; }

QuerySubPlanId ExecutablePipeline::getQuerySubPlanId() const { return querySubPlanId; }

bool ExecutablePipeline::isReconfiguration() const { return reconfiguration; }

ExecutablePipelinePtr ExecutablePipeline::create(uint64_t pipelineId,
                                                 QueryId queryId,
                                                 QuerySubPlanId querySubPlanId,
                                                 const QueryManagerPtr& queryManager,
                                                 const PipelineExecutionContextPtr& pipelineExecutionContext,
                                                 const ExecutablePipelixtagePtr& executablePipelixtage,
                                                 uint32_t numOfProducingPipelix,
                                                 const std::vector<SuccessorExecutablePipeline>& successorPipelix,
                                                 bool reconfiguration) {
    x_ASSERT2_FMT(executablePipelixtage != nullptr,
                    "Executable pipelixtage is null for " << pipelineId
                                                            << "within the following query sub plan: " << querySubPlanId);
    x_ASSERT2_FMT(pipelineExecutionContext != nullptr,
                    "Pipeline context is null for " << pipelineId << "within the following query sub plan: " << querySubPlanId);

    return std::make_shared<ExecutablePipeline>(pipelineId,
                                                queryId,
                                                querySubPlanId,
                                                queryManager,
                                                pipelineExecutionContext,
                                                executablePipelixtage,
                                                numOfProducingPipelix,
                                                successorPipelix,
                                                reconfiguration);
}

void ExecutablePipeline::reconfigure(ReconfigurationMessage& task, WorkerContext& context) {
    x_DEBUG("Going to reconfigure pipeline {} belonging to query id: {} stage id: {}", pipelineId, querySubPlanId, pipelineId);
    Reconfigurable::reconfigure(task, context);
    switch (task.getType()) {
        case ReconfigurationType::Initialize: {
            x_ASSERT2_FMT(isRunning(),
                            "Going to reconfigure a non-running pipeline "
                                << pipelineId << " belonging to query id: " << querySubPlanId << " stage id: " << pipelineId);
            auto refCnt = task.getUserData<uint32_t>();
            context.setObjectRefCnt(this, refCnt);
            break;
        }
        case ReconfigurationType::FailEndOfStream:
        case ReconfigurationType::HardEndOfStream:
        case ReconfigurationType::SoftEndOfStream: {
            if (context.decreaseObjectRefCnt(this) == 1) {
                for (const auto& operatorHandler : pipelineContext->getOperatorHandlers()) {
                    operatorHandler->reconfigure(task, context);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void ExecutablePipeline::postReconfigurationCallback(ReconfigurationMessage& task) {
    x_DEBUG("Going to execute postReconfigurationCallback on pipeline belonging to subplanId: {} stage id: {}",
              querySubPlanId,
              pipelineId);
    Reconfigurable::postReconfigurationCallback(task);
    switch (task.getType()) {
        case ReconfigurationType::FailEndOfStream: {
            auto prevProducerCounter = activeProducers.fetch_sub(1);
            if (prevProducerCounter == 1) {//all producers sent EOS
                for (const auto& operatorHandler : pipelineContext->getOperatorHandlers()) {
                    operatorHandler->postReconfigurationCallback(task);
                }
                // mark the pipeline as failed
                fail();
                // tell the query manager about it
                queryManager->notifyPipelineCompletion(querySubPlanId,
                                                       inherited0::shared_from_this<ExecutablePipeline>(),
                                                       Runtime::QueryTerminationType::Failure);
                for (const auto& successorPipeline : successorPipelix) {
                    if (auto* pipe = std::get_if<ExecutablePipelinePtr>(&successorPipeline)) {
                        auto newReconf = ReconfigurationMessage(queryId, querySubPlanId, task.getType(), *pipe);
                        queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, false);
                        x_DEBUG("Going to reconfigure next pipeline belonging to subplanId: {} stage id: {} got "
                                  "FailEndOfStream with nextPipeline",
                                  querySubPlanId,
                                  (*pipe)->getPipelineId());
                    } else if (auto* sink = std::get_if<DataSinkPtr>(&successorPipeline)) {
                        auto newReconf = ReconfigurationMessage(queryId, querySubPlanId, task.getType(), *sink);
                        queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, false);
                        x_DEBUG("Going to reconfigure next sink belonging to subplanId: {} sink id:{} got FailEndOfStream  "
                                  "with nextPipeline",
                                  querySubPlanId,
                                  (*sink)->toString());
                    }
                }
            }
        }
        case ReconfigurationType::HardEndOfStream:
        case ReconfigurationType::SoftEndOfStream: {
            //we mantain a set of producers, and we will only trigger the end of stream once all producers have sent the EOS, for this we decrement the counter
            auto prevProducerCounter = activeProducers.fetch_sub(1);
            if (prevProducerCounter == 1) {//all producers sent EOS
                x_DEBUG("Reconfiguration of pipeline belonging to subplanId:{} stage id:{} reached prev=1",
                          querySubPlanId,
                          pipelineId);
                auto terminationType = task.getType() == Runtime::ReconfigurationType::SoftEndOfStream
                    ? Runtime::QueryTerminationType::Graceful
                    : Runtime::QueryTerminationType::HardStop;

                // do not change the order here
                // first, stop and drain handlers, if necessary
                for (const auto& operatorHandler : pipelineContext->getOperatorHandlers()) {
                    operatorHandler->stop(terminationType, pipelineContext);
                }
                for (const auto& operatorHandler : pipelineContext->getOperatorHandlers()) {
                    operatorHandler->postReconfigurationCallback(task);
                }
                // second, stop pipeline, if not stopped yet
                stop(terminationType);
                // finally, notify query manager
                queryManager->notifyPipelineCompletion(querySubPlanId,
                                                       inherited0::shared_from_this<ExecutablePipeline>(),
                                                       terminationType);

                for (const auto& successorPipeline : successorPipelix) {
                    if (auto* pipe = std::get_if<ExecutablePipelinePtr>(&successorPipeline)) {
                        auto newReconf = ReconfigurationMessage(queryId, querySubPlanId, task.getType(), *pipe);
                        queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, false);
                        x_DEBUG("Going to reconfigure next pipeline belonging to subplanId: {} stage id: {} got EndOfStream  "
                                  "with nextPipeline",
                                  querySubPlanId,
                                  (*pipe)->getPipelineId());
                    } else if (auto* sink = std::get_if<DataSinkPtr>(&successorPipeline)) {
                        auto newReconf = ReconfigurationMessage(queryId, querySubPlanId, task.getType(), *sink);
                        queryManager->addReconfigurationMessage(queryId, querySubPlanId, newReconf, false);
                        x_DEBUG("Going to reconfigure next sink belonging to subplanId: {} sink id: {} got EndOfStream  with "
                                  "nextPipeline",
                                  querySubPlanId,
                                  (*sink)->toString());
                    }
                }

            } else {
                x_DEBUG("Requested reconfiguration of pipeline belonging to subplanId: {} stage id: {} but refCount was {} "
                          "and now is {}",
                          querySubPlanId,
                          pipelineId,
                          (prevProducerCounter),
                          (prevProducerCounter - 1));
            }
            break;
        }
        default: {
            break;
        }
    }
}

}// namespace x::Runtime::Execution