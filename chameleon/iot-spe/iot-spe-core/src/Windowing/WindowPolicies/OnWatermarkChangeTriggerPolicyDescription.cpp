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

#include <Windowing/WindowPolicies/OnWatermarkChangeTriggerPolicyDescription.hpp>

namespace x::Windowing {

WindowTriggerPolicyPtr OnWatermarkChangeTriggerPolicyDescription::create() {
    return std::make_shared<OnWatermarkChangeTriggerPolicyDescription>(OnWatermarkChangeTriggerPolicyDescription());
}

std::string OnWatermarkChangeTriggerPolicyDescription::toString() { return getTypeAsString(); }

TriggerType OnWatermarkChangeTriggerPolicyDescription::getPolicyType() { return this->policy; }

OnWatermarkChangeTriggerPolicyDescription::OnWatermarkChangeTriggerPolicyDescription()
    : BaseWindowTriggerPolicyDescriptor(TriggerType::triggerOnWatermarkChange) {}

}// namespace x::Windowing