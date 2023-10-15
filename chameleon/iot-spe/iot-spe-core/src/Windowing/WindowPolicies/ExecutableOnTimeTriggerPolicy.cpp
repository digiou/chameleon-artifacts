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

#include <Runtime/WorkerContext.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadNaming.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Windowing/WindowHandler/AbstractJoinHandler.hpp>
#include <Windowing/WindowHandler/AbstractWindowHandler.hpp>
#include <Windowing/WindowPolicies/ExecutableOnTimeTriggerPolicy.hpp>
#include <memory>
namespace x::Windowing {

ExecutableOnTimeTriggerPolicy::~ExecutableOnTimeTriggerPolicy() { x_WARNING("~ExecutableOnTimeTriggerPolicy()"); }

bool ExecutableOnTimeTriggerPolicy::start(AbstractWindowHandlerPtr) { return true; }
bool ExecutableOnTimeTriggerPolicy::start(AbstractWindowHandlerPtr windowHandler, Runtime::WorkerContextRef workerContext) {
    std::unique_lock lock(runningTriggerMutex);
    if (this->running) {
        x_WARNING("ExecutableOnTimeTriggerPolicy::start already started");
        return true;
    }

    this->running = true;
    x_DEBUG("ExecutableOnTimeTriggerPolicy started thread handler={} with ms={}", windowHandler->toString(), triggerTimeInMs);
    std::string handlerName = windowHandler->toString();
    thread = std::make_shared<std::thread>([handlerName, windowHandler, &workerContext, this]() {
        setThreadName("whdlr-%d", handlerName.c_str());
        while (this->running) {
            x_DEBUG("ExecutableOnTimeTriggerPolicy:: trigger policy now");
            std::this_thread::sleep_for(std::chrono::milliseconds(triggerTimeInMs));
            if (windowHandler != nullptr) {
                //                x_ASSERT(false, "This function should not be called");
                windowHandler->trigger(workerContext);
            }
        }
    });
    return true;
}

bool ExecutableOnTimeTriggerPolicy::start(Join::AbstractJoinHandlerPtr) { return true; }

bool ExecutableOnTimeTriggerPolicy::start(Join::AbstractJoinHandlerPtr joinHandler, Runtime::WorkerContextRef workerContext) {
    std::unique_lock lock(runningTriggerMutex);
    if (this->running) {
        x_WARNING("ExecutableOnTimeTriggerPolicy::start already started");
        return true;
    }

    this->running = true;
    x_DEBUG("ExecutableOnTimeTriggerPolicy started thread handler={} with ms={}", joinHandler->toString(), triggerTimeInMs);
    std::string handlerName = joinHandler->toString();
    thread = std::make_shared<std::thread>([handlerName, joinHandler, &workerContext, this]() {
        setThreadName("whdlr-%d", handlerName.c_str());
        while (this->running) {
            x_DEBUG("ExecutableOnTimeTriggerPolicy:: trigger policy now");
            if (joinHandler != nullptr) {
                //                x_ASSERT(false, "This function should not be called");
                joinHandler->trigger(workerContext);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(triggerTimeInMs));
        }
    });
    thread->detach();
    return true;
}

bool ExecutableOnTimeTriggerPolicy::stop() {
    std::unique_lock lock(runningTriggerMutex);
    x_DEBUG("ExecutableOnTimeTriggerPolicy: Stop called");
    if (!this->running) {
        x_DEBUG("ExecutableOnTimeTriggerPolicy: Stop called but was already not running");
        return true;
    }
    this->running = false;
    lock.unlock();
    if (thread && thread->joinable()) {
        thread->join();
        x_DEBUG("ExecutableOnTimeTriggerPolicy: Thread joined");
    }
    thread.reset();
    // TODO what happens to the content of the window that it is still in the state?
    return true;
}

ExecutableOnTimeTriggerPolicy::ExecutableOnTimeTriggerPolicy(uint64_t triggerTimeInMs) : triggerTimeInMs(triggerTimeInMs) {}

ExecutableOnTimeTriggerPtr ExecutableOnTimeTriggerPolicy::create(uint64_t triggerTimeInMs) {
    return std::make_shared<ExecutableOnTimeTriggerPolicy>(triggerTimeInMs);
}

}// namespace x::Windowing
