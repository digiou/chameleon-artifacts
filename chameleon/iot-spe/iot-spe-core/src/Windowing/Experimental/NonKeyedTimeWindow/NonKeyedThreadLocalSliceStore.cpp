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

#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSlice.hpp>
#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedThreadLocalSliceStore.hpp>
#include <memory>

namespace x::Windowing::Experimental {

NonKeyedThreadLocalSliceStore::NonKeyedThreadLocalSliceStore(uint64_t entrySize, uint64_t windowSize, uint64_t windowSlide)
    : ThreadLocalSliceStore(windowSize, windowSlide), entrySize(entrySize) {}

NonKeyedSlicePtr NonKeyedThreadLocalSliceStore::allocateNewSlice(uint64_t startTs, uint64_t endTs) {
    return std::make_unique<NonKeyedSlice>(entrySize, startTs, endTs);
}

}// namespace x::Windowing::Experimental