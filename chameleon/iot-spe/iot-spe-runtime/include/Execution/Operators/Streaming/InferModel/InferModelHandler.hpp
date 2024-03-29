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

#ifndef x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_INFERMODEL_INFERMODELHANDLER_HPP_
#define x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_INFERMODEL_INFERMODELHANDLER_HPP_

#include <Execution/Operators/Streaming/InferModel/TensorflowAdapter.hpp>
#include <Runtime/Execution/OperatorHandler.hpp>
#include <Runtime/Reconfigurable.hpp>

namespace x::Runtime::Execution::Operators {

class InferModelHandler;
using InferModelHandlerPtr = std::shared_ptr<InferModelHandler>;

/**
 * @brief Operator handler for inferModel.
 */
class InferModelHandler : public OperatorHandler {
  public:
    explicit InferModelHandler(const std::string& model);

    static InferModelHandlerPtr create(const std::string& model);

    ~InferModelHandler() override = default;

    void start(Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext,
               Runtime::StateManagerPtr stateManager,
               uint32_t localStateVariableId) override;

    void stop(Runtime::QueryTerminationType queryTerminationType,
              Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext) override;

    void reconfigure(Runtime::ReconfigurationMessage& task, Runtime::WorkerContext& context) override;

    void postReconfigurationCallback(Runtime::ReconfigurationMessage& task) override;

    const std::string& getModel() const;

    const TensorflowAdapterPtr& getTensorflowAdapter() const;

  private:
    std::string model;
    TensorflowAdapterPtr tfAdapter;
};
}// namespace x::Runtime::Execution::Operators

#endif// x_RUNTIME_INCLUDE_EXECUTION_OPERATORS_STREAMING_INFERMODEL_INFERMODELHANDLER_HPP_
