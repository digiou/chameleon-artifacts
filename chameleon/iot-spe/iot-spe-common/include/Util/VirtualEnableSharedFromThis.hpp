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
#ifndef x_COMMON_INCLUDE_UTIL_VIRTUALENABLESHAREDFROMTHIS_HPP_
#define x_COMMON_INCLUDE_UTIL_VIRTUALENABLESHAREDFROMTHIS_HPP_

#include <cstddef>
#include <memory>

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#define x_NOEXCEPT(isNoexcept) noexcept(isNoexcept)
#else
#define x_NOEXCEPT(isNoexcept)
#endif

namespace x::detail {
/// base class for enabling enable_shared_from_this in classes with multiple super-classes that inherit enable_shared_from_this
template<bool isNoexceptDestructible>
struct virtual_enable_shared_from_this_base
    : std::enable_shared_from_this<virtual_enable_shared_from_this_base<isNoexceptDestructible>> {
    virtual ~virtual_enable_shared_from_this_base() x_NOEXCEPT(isNoexceptDestructible) = default;
};

/// concrete class for enabling enable_shared_from_this in classes with multiple super-classes that inherit enable_shared_from_this
template<typename T, bool isNoexceptDestructible = true>
struct virtual_enable_shared_from_this : virtual virtual_enable_shared_from_this_base<isNoexceptDestructible> {

    ~virtual_enable_shared_from_this() x_NOEXCEPT(isNoexceptDestructible) override = default;

    template<typename T1 = T>
    std::shared_ptr<T1> shared_from_this() {
        return std::dynamic_pointer_cast<T1>(virtual_enable_shared_from_this_base<isNoexceptDestructible>::shared_from_this());
    }

    template<typename T1 = T>
    std::weak_ptr<T1> weak_from_this() {
        return std::dynamic_pointer_cast<T1>(
            virtual_enable_shared_from_this_base<isNoexceptDestructible>::weak_from_this().lock());
    }

    template<class Down>
    std::shared_ptr<Down> downcast_shared_from_this() {
        return std::dynamic_pointer_cast<Down>(virtual_enable_shared_from_this_base<isNoexceptDestructible>::shared_from_this());
    }
};

}// namespace x::detail

#endif// x_COMMON_INCLUDE_UTIL_VIRTUALENABLESHAREDFROMTHIS_HPP_
