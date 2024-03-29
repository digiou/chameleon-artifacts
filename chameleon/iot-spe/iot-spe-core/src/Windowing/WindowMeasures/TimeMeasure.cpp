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

#include <Windowing/WindowMeasures/TimeMeasure.hpp>

namespace x::Windowing {

TimeMeasure::TimeMeasure(uint64_t ms) : ms(ms) {}

uint64_t TimeMeasure::getTime() const { return ms; }

std::string TimeMeasure::toString() { return "TimeMeasure: ms)" + std::to_string(ms); }

bool TimeMeasure::equals(const TimeMeasure& other) const { return this->ms == other.ms; }

}// namespace x::Windowing