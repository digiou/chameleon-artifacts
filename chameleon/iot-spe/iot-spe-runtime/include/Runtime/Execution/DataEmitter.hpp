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

#ifndef x_RUNTIME_INCLUDE_RUNTIME_EXECUTION_DATAEMITTER_HPP_
#define x_RUNTIME_INCLUDE_RUNTIME_EXECUTION_DATAEMITTER_HPP_

#include <Runtime/QueryTerminationType.hpp>
#include <Runtime/RuntimeEventListener.hpp>
#include <Util/VirtualEnableSharedFromThis.hpp>
namespace x {
namespace Runtime {
class TupleBuffer;
}

/**
 * @brief Interface that classes have to adhere to emit data as tasks
 */
class DataEmitter : public Runtime::RuntimeEventListener {
  public:
    /**
     * @brief create a task using the provided buffer and submit it to a task consumer, e.g., query manager
     * @param buffer
     */
    virtual void emitWork(Runtime::TupleBuffer& buffer) = 0;

    virtual ~DataEmitter() x_NOEXCEPT(false) = default;

    /**
     * @brief
     */
    virtual void onEndOfStream(Runtime::QueryTerminationType) {}

    /**
     * @brief
     */
    virtual void onEvent(Runtime::BaseEvent&) override {}
};
}// namespace x
#endif// x_RUNTIME_INCLUDE_RUNTIME_EXECUTION_DATAEMITTER_HPP_
