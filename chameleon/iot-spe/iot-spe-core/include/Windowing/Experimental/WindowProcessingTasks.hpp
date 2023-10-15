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
#ifndef x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_WINDOWPROCESSINGTASKS_HPP_
#define x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_WINDOWPROCESSINGTASKS_HPP_
#include <cinttypes>
namespace x::Windowing::Experimental {

/**
 * @brief This task models the merge task of an a specific slice, with a start and a end.
 */
class SliceMergeTask {
  public:
    uint64_t sequenceNumber;
    uint64_t startSlice;
    uint64_t endSlice;
};

/**
 * @brief This task models the trigger of a window with a start and a end.
 */
class WindowTriggerTask {
  public:
    uint64_t sequenceNumber;
    uint64_t windowStart;
    uint64_t windowEnd;
};

struct Window {
    uint64_t startTs;
    uint64_t endTs;
    uint64_t sequenceNumber;
};

}// namespace x::Windowing::Experimental

#endif// x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_WINDOWPROCESSINGTASKS_HPP_