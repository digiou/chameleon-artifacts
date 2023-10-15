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
#include <QueryCompiler/Operators/PhysicalOperators/CEP/CEPOperatorHandler/CEPOperatorHandler.hpp>
#include <State/StateManager.hpp>

namespace x::CEP {

CEPOperatorHandlerPtr CEPOperatorHandler::create() { return std::make_shared<x::CEP::CEPOperatorHandler>(); }

void CEPOperatorHandler::start(Runtime::Execution::PipelineExecutionContextPtr context,
                               Runtime::StateManagerPtr,
                               uint32_t localStateVariableId) {
    x_DEBUG("CEPOperatorHandler::start() with localStateVariableId {} {}", localStateVariableId, fmt::ptr(context));
    this->clearCounter();
}

void CEPOperatorHandler::stop(Runtime::QueryTerminationType, Runtime::Execution::PipelineExecutionContextPtr) {}

void CEPOperatorHandler::reconfigure(Runtime::ReconfigurationMessage& task, Runtime::WorkerContext& context) {
    Reconfigurable::reconfigure(task, context);
}

void CEPOperatorHandler::postReconfigurationCallback(Runtime::ReconfigurationMessage& task) {
    Reconfigurable::postReconfigurationCallback(task);
}

[[maybe_unused]] void CEPOperatorHandler::incrementCounter() { counter++; }

[[maybe_unused]] uint64_t CEPOperatorHandler::getCounter() { return counter; }

void CEPOperatorHandler::clearCounter() { counter = 0; }
}// namespace x::CEP