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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SOURCES_NETWORKSOURCEDESCRIPTOR_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SOURCES_NETWORKSOURCEDESCRIPTOR_HPP_

#include <Network/xPartition.hpp>
#include <Network/NodeLocation.hpp>
#include <Operators/LogicalOperators/Sources/SourceDescriptor.hpp>
#include <chrono>
#include <string>
namespace x {
namespace Network {

/**
 * @brief Descriptor defining properties used for creating physical zmq source inside the network stack
 */
class NetworkSourceDescriptor : public SourceDescriptor {

  public:
    /**
     * @brief The constructor for the network source descriptor
     * @param schema
     * @param xPartition
     * @param nodeLocation
     * @return instance of network source descriptor
     */
    static SourceDescriptorPtr create(SchemaPtr schema,
                                      xPartition xPartition,
                                      NodeLocation nodeLocation,
                                      std::chrono::milliseconds waitTime,
                                      uint32_t retryTimes);

    /**
     * @brief equal method for the NetworkSourceDescriptor
     * @param other
     * @return true if equal, else false
     */
    [[nodiscard]] bool equal(SourceDescriptorPtr const& other) const override;

    /**
     * @brief creates a string representation of the NetworkSourceDescriptor
     * @return the string representation
     */
    std::string toString() const override;

    /**
     * @brief getter for the x partition
     * @return the xPartition
     */
    xPartition getxPartition() const;

    NodeLocation getNodeLocation() const;

    /**
     * @brief getter for the wait time
     * @return the wait time
     */
    std::chrono::milliseconds getWaitTime() const;

    /**
     * @brief getter for the retry times
     * @return the retry times
     */
    uint8_t getRetryTimes() const;

    SourceDescriptorPtr copy() override;

  private:
    explicit NetworkSourceDescriptor(SchemaPtr schema,
                                     xPartition xPartition,
                                     NodeLocation nodeLocation,
                                     std::chrono::milliseconds waitTime,
                                     uint32_t retryTimes);

    xPartition xPartition;
    NodeLocation nodeLocation;
    std::chrono::milliseconds waitTime;
    uint32_t retryTimes;
};

using networkSourceDescriptorPtr = std::shared_ptr<NetworkSourceDescriptor>;

}// namespace Network
}// namespace x

#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SOURCES_NETWORKSOURCEDESCRIPTOR_HPP_
