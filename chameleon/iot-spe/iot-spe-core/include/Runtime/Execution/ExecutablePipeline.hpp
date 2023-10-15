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

#ifndef x_CORE_INCLUDE_RUNTIME_EXECUTION_EXECUTABLEPIPELINE_HPP_
#define x_CORE_INCLUDE_RUNTIME_EXECUTION_EXECUTABLEPIPELINE_HPP_
#include <Common/Identifiers.hpp>
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/ExecutionResult.hpp>
#include <Runtime/QueryTerminationType.hpp>
#include <Runtime/Reconfigurable.hpp>
#include <Runtime/RuntimeEventListener.hpp>
#include <Runtime/RuntimeForwardRefs.hpp>
#include <atomic>
#include <memory>
#include <variant>
#include <vector>

namespace x::Runtime::Execution {

/**
 * @brief An ExecutablePipeline represents a fragment of an overall query.
 * It can contain multiple operators and the implementation of its computation is defined in the ExecutablePipelixtage.
 * Furthermore, it holds the PipelineExecutionContextPtr and a reference to the next pipeline in the query plan.
 */
class ExecutablePipeline : public Reconfigurable, public Runtime::RuntimeEventListener {
    // virtual_enable_shared_from_this necessary for double inheritance of enable_shared_from_this
    using inherited0 = Reconfigurable;
    using inherited1 = Runtime::RuntimeEventListener;

    friend class QueryManager;

    enum class Pipelixtatus : uint8_t { PipelineCreated, PipelineRunning, Pipelixtopped, PipelineFailed };

  public:
    /**
     * @brief Constructor for an executable pipeline.
     * @param pipelineId The Id of this pipeline
     * @param querySubPlanId the id of the query sub plan
     * @param queryManager reference to the queryManager
     * @param pipelineContext the pipeline context
     * @param executablePipelixtage the executable pipeline stage
     * @param numOfProducingPipelix number of producing pipelix
     * @param successorPipelix a vector of successor pipelix
     * @param reconfiguration indicates if this is a reconfiguration task. Default = false.
     * @return ExecutablePipelinePtr
     */
    explicit ExecutablePipeline(uint64_t pipelineId,
                                QueryId queryId,
                                QuerySubPlanId qepId,
                                QueryManagerPtr queryManager,
                                PipelineExecutionContextPtr pipelineExecutionContext,
                                ExecutablePipelixtagePtr executablePipelixtage,
                                uint32_t numOfProducingPipelix,
                                std::vector<SuccessorExecutablePipeline> successorPipelix,
                                bool reconfiguration);

    /**
     * @brief Factory method to create a new executable pipeline.
     * @param pipelineId The Id of this pipeline
     * @param querySubPlanId the id of the query sub plan
     * @param pipelineContext the pipeline context
     * @param executablePipelixtage the executable pipeline stage
     * @param numOfProducingPipelix number of producing pipelix
     * @param successorPipelix a vector of successor pipelix
     * @param reconfiguration indicates if this is a reconfiguration task. Default = false.
     * @return ExecutablePipelinePtr
     */
    static ExecutablePipelinePtr create(uint64_t pipelineId,
                                        QueryId queryId,
                                        QuerySubPlanId querySubPlanId,
                                        const QueryManagerPtr& queryManager,
                                        const PipelineExecutionContextPtr& pipelineExecutionContext,
                                        const ExecutablePipelixtagePtr& executablePipelixtage,
                                        uint32_t numOfProducingPipelix,
                                        const std::vector<SuccessorExecutablePipeline>& successorPipelix,
                                        bool reconfiguration = false);

    /**
     * @brief Execute a pipeline stage
     * @param inputBuffer: the input buffer on which to execute the pipeline stage
     * @param workerContext: the worker context
     * @return true if no error occurred
     */
    ExecutionResult execute(TupleBuffer& inputBuffer, WorkerContextRef workerContext);

    /**
   * @brief Initialises a pipeline stage
   * @return boolean if successful
   */
    bool setup(const QueryManagerPtr& queryManager, const BufferManagerPtr& bufferManager);

    /**
     * @brief Starts a pipeline stage and passes statemanager and local state counter further to the operator handler
     * @param stateManager pointer to the current state manager
     * @return Success if pipeline stage started 
     */
    bool start(const StateManagerPtr& stateManager);

    /**
     * @brief Stops pipeline stage
     * @param terminationType indicates the termination type see @QueryTerminationType
     * @return  Success if pipeline stage stopped
     */
    bool stop(QueryTerminationType terminationType);

    /**
     * @brief Fails pipeline stage
     * @return true if successful
     */
    bool fail();

    /**
    * @brief Get id of pipeline stage
    * @return pipeline id
    */
    uint64_t getPipelineId() const;

    /**
     * @brief Get query sub plan id.
     * @return QuerySubPlanId.
     */
    QuerySubPlanId getQuerySubPlanId() const;

    /**
     * @brief Checks if this pipeline is running
     * @return true if pipeline is running.
     */
    bool isRunning() const;

    /**
    * @return returns true if the pipeline contains a function pointer for a reconfiguration task
    */
    bool isReconfiguration() const;

    /**
     * @brief reconfigure callback called upon a reconfiguration
     * @param task the reconfig descriptor
     * @param context the worker context
     */
    void reconfigure(ReconfigurationMessage& task, WorkerContext& context) override;

    /**
     * @brief final reconfigure callback called upon a reconfiguration
     * @param task the reconfig descriptor
     */
    void postReconfigurationCallback(ReconfigurationMessage& task) override;

    /**
     * @brief atomically increment number of producers for this pipeline
     */
    void incrementProducerCount();

    /**
     * @brief Get query plan id.
     * @return QueryId.
     */
    QueryId getQueryId() const;

    /**
     * @brief Gets the successor pipelix
     * @return SuccessorPipelix
     */
    const std::vector<SuccessorExecutablePipeline>& getSuccessors() const;

    /**
     * @brief API method called upon receiving an event (from downstream)
     * @param event
     */
    void onEvent(Runtime::BaseEvent& event) override;

    /**
     * @brief API method called upon receiving an event (from downstream)
     * @param event
     */
    void onEvent(Runtime::BaseEvent& event, Runtime::WorkerContextRef);

    PipelineExecutionContextPtr getContext() { return pipelineContext; };

  private:
    const uint64_t pipelineId;
    const QueryId queryId;
    const QuerySubPlanId querySubPlanId;
    QueryManagerPtr queryManager;
    ExecutablePipelixtagePtr executablePipelixtage;
    PipelineExecutionContextPtr pipelineContext;
    bool reconfiguration;
    std::atomic<Pipelixtatus> pipelixtatus;
    std::atomic<uint32_t> activeProducers = 0;
    std::vector<SuccessorExecutablePipeline> successorPipelix;
};

}// namespace x::Runtime::Execution

#endif// x_CORE_INCLUDE_RUNTIME_EXECUTION_EXECUTABLEPIPELINE_HPP_
