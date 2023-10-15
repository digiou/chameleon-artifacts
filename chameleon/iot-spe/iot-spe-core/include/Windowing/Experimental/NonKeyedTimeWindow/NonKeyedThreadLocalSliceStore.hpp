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

#ifndef x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALSLICESTORE_HPP_
#define x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALSLICESTORE_HPP_
#include <Windowing/Experimental/ThreadLocalSliceStore.hpp>
#include <memory>

namespace x::Windowing::Experimental {

class NonKeyedSlice;
using NonKeyedSlicePtr = std::unique_ptr<NonKeyedSlice>;

/**
 * @brief A thread local slice store for global (non-keyed) tumbling and sliding windows,
 * which stores slices for a specific thread.
 * In the current implementation we handle tumbling windows as sliding widows with windowSize==windowSlide.
 * As the slice store is only using by a single thread, we don't have to protect its functions for concurrent accesses.
 */
class NonKeyedThreadLocalSliceStore : public ThreadLocalSliceStore<NonKeyedSlice> {
  public:
    explicit NonKeyedThreadLocalSliceStore(uint64_t entrySize, uint64_t windowSize, uint64_t windowSlide);
    ~NonKeyedThreadLocalSliceStore() = default;

  private:
    NonKeyedSlicePtr allocateNewSlice(uint64_t startTs, uint64_t endTs) override;
    const uint64_t entrySize;
};
}// namespace x::Windowing::Experimental
#endif// x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_NONKEYEDTIMEWINDOW_NONKEYEDTHREADLOCALSLICESTORE_HPP_