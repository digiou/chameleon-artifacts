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

#include <Windowing/WindowActions/BaseWindowActionDescriptor.hpp>

namespace x::Windowing {

BaseWindowActionDescriptor::BaseWindowActionDescriptor(ActionType action) : action(action) {}

std::string BaseWindowActionDescriptor::toString() { return getTypeAsString(); }

std::string BaseWindowActionDescriptor::getTypeAsString() {
    if (action == ActionType::WindowAggregationTriggerAction) {
        return "WindowAggregationTriggerAction";
    }
    if (action == ActionType::SliceAggregationTriggerAction) {
        return "SliceAggregationTriggerAction";
    } else {
        return "Unknown Action";
    }
}

}// namespace x::Windowing