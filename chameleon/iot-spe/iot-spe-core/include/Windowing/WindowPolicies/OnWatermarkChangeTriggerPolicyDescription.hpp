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

#ifndef x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_ONWATERMARKCHANGETRIGGERPOLICYDESCRIPTION_HPP_
#define x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_ONWATERMARKCHANGETRIGGERPOLICYDESCRIPTION_HPP_

#include <Windowing/WindowPolicies/BaseWindowTriggerPolicyDescriptor.hpp>

namespace x::Windowing {

class OnWatermarkChangeTriggerPolicyDescription : public BaseWindowTriggerPolicyDescriptor {
  public:
    static WindowTriggerPolicyPtr create();
    ~OnWatermarkChangeTriggerPolicyDescription() override = default;
    TriggerType getPolicyType() override;
    std::string toString() override;

  private:
    OnWatermarkChangeTriggerPolicyDescription();
};
}// namespace x::Windowing
#endif// x_CORE_INCLUDE_WINDOWING_WINDOWPOLICIES_ONWATERMARKCHANGETRIGGERPOLICYDESCRIPTION_HPP_
