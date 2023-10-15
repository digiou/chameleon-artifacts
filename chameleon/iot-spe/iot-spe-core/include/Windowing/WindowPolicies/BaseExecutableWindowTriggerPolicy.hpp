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

#ifndef x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_BASEEXECUTABLEWINDOWTRIGGERPOLICY_HPP_
#define x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_BASEEXECUTABLEWINDOWTRIGGERPOLICY_HPP_
#include <Windowing/JoinForwardRefs.hpp>
#include <Windowing/WindowingForwardRefs.hpp>
namespace x::Windowing {

class BaseExecutableWindowTriggerPolicy {
  public:
    virtual ~BaseExecutableWindowTriggerPolicy() = default;
    /**
     * @brief This function starts the trigger policy
     * @param windowHandler
     * @param workerContext to allocate buffers
     * @return bool indicating success
     */
    virtual bool start(AbstractWindowHandlerPtr windowHandler, Runtime::WorkerContextRef workerContext) = 0;

    /**
   * @brief This function starts the trigger policy
    * @param windowHandler
    * @param workerContext to allocate buffers
   * @return bool indicating success
   */
    virtual bool start(Join::AbstractJoinHandlerPtr joinHandler, Runtime::WorkerContextRef workerContext) = 0;

    /**
     * @brief This function starts the trigger policy
     * @return bool indicating success
     */
    virtual bool start(AbstractWindowHandlerPtr windowHandler) = 0;

    /**
   * @brief This function starts the trigger policy
   * @return bool indicating success
   */
    virtual bool start(Join::AbstractJoinHandlerPtr joinHandler) = 0;

    /**
    * @brief This function stop the trigger policy
    * @return bool indicating success
    */
    virtual bool stop() = 0;
};
}// namespace x::Windowing
#endif// x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_BASEEXECUTABLEWINDOWTRIGGERPOLICY_HPP_
