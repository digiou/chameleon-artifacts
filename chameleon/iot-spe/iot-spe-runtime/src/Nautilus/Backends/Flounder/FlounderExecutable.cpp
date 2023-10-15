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

#include <Nautilus/Backends/Flounder/FlounderExecutable.hpp>
#include <Util/Logger/Logger.hpp>
#include <flounder/executable.h>

namespace x::Nautilus::Backends::Flounder {
FlounderExecutable::FlounderExecutable(std::unique_ptr<flounder::Executable> engine) : engine(std::move(engine)) {}

void* FlounderExecutable::getInvocableFunctionPtr(const std::string&) { return reinterpret_cast<void*>(engine->callback()); }
bool FlounderExecutable::hasInvocableFunctionPtr() { return true; }

FlounderExecutable::~FlounderExecutable() noexcept = default;
}// namespace x::Nautilus::Backends::Flounder