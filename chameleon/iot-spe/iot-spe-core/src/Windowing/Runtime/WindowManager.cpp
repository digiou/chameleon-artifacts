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

#include <Util/Core.hpp>
#include <Windowing/Runtime/WindowManager.hpp>
namespace x::Windowing {

WindowManager::WindowManager(Windowing::WindowTypePtr windowType, uint64_t allowedLatexs, uint64_t id)
    : allowedLatexs(allowedLatexs), windowType(std::move(windowType)), id(id) {
    x_DEBUG("WindowManager() with id={}", id);
}

Windowing::WindowTypePtr WindowManager::getWindowType() { return windowType; }

uint64_t WindowManager::getAllowedLatexs() const {
    x_DEBUG("getAllowedLatexs={}", allowedLatexs);
    return allowedLatexs;
}

}// namespace x::Windowing