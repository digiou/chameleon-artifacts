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

#include <Operators/LogicalOperators/LogicalUnaryOperatorNode.hpp>
#include <Optimizer/Phases/TypeInferencePhaseContext.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/WindowTypes/ContentBasedWindowType.hpp>
#include <Windowing/WindowTypes/ThresholdWindow.hpp>

namespace x::Windowing {

ContentBasedWindowType::ContentBasedWindowType() = default;

bool ContentBasedWindowType::isContentBasedWindowType() { return true; }

ThresholdWindowPtr ContentBasedWindowType::asThresholdWindow(ContentBasedWindowTypePtr contentBasedWindowType) {
    if (auto thresholdWindow = std::dynamic_pointer_cast<ThresholdWindow>(contentBasedWindowType)) {
        return thresholdWindow;
    } else {
        x_ERROR("Can not cast the content based window type to a threshold window");
        x_THROW_RUNTIME_ERROR("Can not cast the content based window type to a threshold window");
    }
}
}// namespace x::Windowing