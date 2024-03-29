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

#include <Windowing/Experimental/KeyedTimeWindow/KeyedSlice.hpp>
#include <Windowing/Experimental/KeyedTimeWindow/KeyedThreadLocalSliceStore.hpp>

namespace x::Windowing::Experimental {
KeyedThreadLocalSliceStore::KeyedThreadLocalSliceStore(x::Experimental::HashMapFactoryPtr hashMapFactory,
                                                       uint64_t windowSize,
                                                       uint64_t windowSlide)
    : ThreadLocalSliceStore<KeyedSlice>(windowSize, windowSlide), hashMapFactory(hashMapFactory){};

KeyedSlicePtr KeyedThreadLocalSliceStore::allocateNewSlice(uint64_t startTs, uint64_t endTs) {
    return std::make_unique<KeyedSlice>(hashMapFactory, startTs, endTs);
}

}// namespace x::Windowing::Experimental