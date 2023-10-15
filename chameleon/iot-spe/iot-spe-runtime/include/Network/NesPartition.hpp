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

#ifndef x_RUNTIME_INCLUDE_NETWORK_xPARTITION_HPP_
#define x_RUNTIME_INCLUDE_NETWORK_xPARTITION_HPP_
#include <Common/Identifiers.hpp>
#include <Util/Logger/Logger.hpp>
#include <cstdint>
#include <string>

namespace x::Network {
static constexpr uint16_t DEFAULT_NUM_SERVER_THREADS = 3;

class xPartition {
  public:
    explicit xPartition(QueryId queryId, OperatorId operatorId, PartitionId partitionId, SubpartitionId subpartitionId)
        : queryId(queryId), operatorId(operatorId), partitionId(partitionId), subpartitionId(subpartitionId) {}

    /**
     * @brief getter for the queryId
     * @return the queryId
     */
    [[nodiscard]] QueryId getQueryId() const { return queryId; }

    /**
     * @brief getter for the operatorId
     * @return the operatorId
     */
    [[nodiscard]] OperatorId getOperatorId() const { return operatorId; }

    /**
     * @brief getter for the partitionId
     * @return the partitionId
     */
    [[nodiscard]] PartitionId getPartitionId() const { return partitionId; }

    /**
     * @brief getter for the getSubpartitionId
     * @return the subpartitionId
     */
    [[nodiscard]] SubpartitionId getSubpartitionId() const { return subpartitionId; }

    [[nodiscard]] std::string toString() const {
        return std::to_string(queryId) + "::" + std::to_string(operatorId) + "::" + std::to_string(partitionId)
            + "::" + std::to_string(subpartitionId);
    }

    friend std::ostream& operator<<(std::ostream& os, const xPartition& partition) {
        os << partition.toString();
        return os;
    }

    /**
     * @brief The equals operator for the xPartition. It is not comparing threadIds
     * @param lhs
     * @param rhs
     * @return
     */
    friend bool operator==(const xPartition& lhs, const xPartition& rhs) {
        return lhs.queryId == rhs.queryId && lhs.operatorId == rhs.operatorId && lhs.partitionId == rhs.partitionId
            && lhs.subpartitionId == rhs.subpartitionId;
    }

    friend bool operator<(const xPartition& lhs, const xPartition& rhs) {
        return lhs.queryId < rhs.queryId && lhs.operatorId < rhs.operatorId && lhs.partitionId < rhs.partitionId
            && lhs.subpartitionId < rhs.subpartitionId;
    }

  private:
    const QueryId queryId;
    const OperatorId operatorId;
    const PartitionId partitionId;
    const SubpartitionId subpartitionId;
};
}// namespace x::Network
namespace std {
template<>
struct hash<x::Network::xPartition> {
    std::uint64_t operator()(const x::Network::xPartition& k) const {
        using std::hash;

        // Hash function for the xPartition
        // Compute individual hash values of the Ints and combine them using XOR and bit shifting:
        return ((hash<uint64_t>()(k.getQueryId()) ^ (hash<uint64_t>()(k.getOperatorId()) << 1)) >> 1)
            ^ ((hash<uint64_t>()(k.getPartitionId()) ^ (hash<uint64_t>()(k.getSubpartitionId()) << 1)) >> 1);
    }
};
}// namespace std

namespace fmt {
template<>
struct formatter<x::Network::xPartition> : formatter<std::string> {
    auto format(const x::Network::xPartition& partition, format_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(),
                              "query Id:{} OperatorId:{} PartitionId: {} SubpartitionID: {}",
                              partition.getQueryId(),
                              partition.getOperatorId(),
                              partition.getPartitionId(),
                              partition.getSubpartitionId());
    }
};
}//namespace fmt
#endif// x_RUNTIME_INCLUDE_NETWORK_xPARTITION_HPP_
