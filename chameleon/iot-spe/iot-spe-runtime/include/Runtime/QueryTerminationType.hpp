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

#ifndef x_RUNTIME_INCLUDE_RUNTIME_QUERYTERMINATIONTYPE_HPP_
#define x_RUNTIME_INCLUDE_RUNTIME_QUERYTERMINATIONTYPE_HPP_

#include <Util/Logger/Logger.hpp>
#include <ostream>
#include <stdint.h>

namespace x::Runtime {

enum class QueryTerminationType : uint8_t { Graceful = 0, HardStop, Failure, Invalid };

template<typename O = std::ostream>
static O& operator<<(O& os, const QueryTerminationType& type) {
    switch (type) {
        case QueryTerminationType::Graceful: return os << "Graceful";
        case QueryTerminationType::HardStop: return os << "HardStop";
        case QueryTerminationType::Failure: return os << "Failure";
        default: return os << "Invalid";
    }
}

}// namespace x::Runtime

namespace fmt {
template<>
struct formatter<x::Runtime::QueryTerminationType> : formatter<std::string> {
    auto format(const x::Runtime::QueryTerminationType& termination_type, format_context& ctx) -> decltype(ctx.out()) {
        switch (termination_type) {
            case x::Runtime::QueryTerminationType::Graceful: return fmt::format_to(ctx.out(), "Graceful");
            case x::Runtime::QueryTerminationType::HardStop: return fmt::format_to(ctx.out(), "HardStop");
            case x::Runtime::QueryTerminationType::Failure: return fmt::format_to(ctx.out(), "Failure");
            default: return fmt::format_to(ctx.out(), "Invalid");
        }
    }
};
}//namespace fmt

#endif// x_RUNTIME_INCLUDE_RUNTIME_QUERYTERMINATIONTYPE_HPP_
