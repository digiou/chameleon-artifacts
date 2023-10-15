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

#ifndef x_RUNTIME_INCLUDE_NETWORK_CHANNELID_HPP_
#define x_RUNTIME_INCLUDE_NETWORK_CHANNELID_HPP_

#include <Network/xPartition.hpp>
#include <fmt/core.h>

namespace x::Network {

class ChannelId {
  public:
    explicit ChannelId(xPartition xPartition, uint32_t threadId) : xPartition(xPartition), threadId(threadId) {
        // nop
    }

    [[nodiscard]] xPartition getxPartition() const { return xPartition; }

    [[nodiscard]] uint64_t getThreadId() const { return threadId; }

    [[nodiscard]] std::string toString() const { return xPartition.toString() + "(threadId=" + std::to_string(threadId) + ")"; }

    friend std::ostream& operator<<(std::ostream& os, const ChannelId& channelId) { return os << channelId.toString(); }

  private:
    const xPartition xPartition;
    const uint32_t threadId;
};
}// namespace x::Network

namespace fmt {
template<>
struct formatter<x::Network::ChannelId> : formatter<std::string> {
    auto format(const x::Network::ChannelId& channel_id, format_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}:{}", channel_id.getThreadId(), channel_id.getxPartition().toString());
    }
};
}// namespace fmt

#endif// x_RUNTIME_INCLUDE_NETWORK_CHANNELID_HPP_
