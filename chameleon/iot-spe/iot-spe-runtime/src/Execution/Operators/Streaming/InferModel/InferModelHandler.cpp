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

#include <Execution/Operators/Streaming/InferModel/InferModelHandler.hpp>
#include <Execution/Operators/Streaming/InferModel/TensorflowAdapter.hpp>
#include <Runtime/WorkerContext.hpp>

namespace x::Runtime::Execution::Operators {

InferModelHandlerPtr InferModelHandler::create(const std::string& model) { return std::make_shared<InferModelHandler>(model); }

InferModelHandler::InferModelHandler(const std::string& model) {
    this->model = model;
    tfAdapter = TensorflowAdapter::create();
    tfAdapter->initializeModel(model);
}

void InferModelHandler::start(Runtime::Execution::PipelineExecutionContextPtr, Runtime::StateManagerPtr, uint32_t) {}

void InferModelHandler::stop(Runtime::QueryTerminationType, Runtime::Execution::PipelineExecutionContextPtr) {}

void InferModelHandler::reconfigure(Runtime::ReconfigurationMessage&, Runtime::WorkerContext&) {}

void InferModelHandler::postReconfigurationCallback(Runtime::ReconfigurationMessage&) {}

const std::string& InferModelHandler::getModel() const { return model; }

const TensorflowAdapterPtr& InferModelHandler::getTensorflowAdapter() const { return tfAdapter; }

}// namespace x::Runtime::Execution::Operators
