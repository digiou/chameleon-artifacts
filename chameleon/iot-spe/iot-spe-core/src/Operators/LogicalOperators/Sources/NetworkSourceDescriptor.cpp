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

#include <API/Schema.hpp>
#include <Operators/LogicalOperators/Sources/NetworkSourceDescriptor.hpp>
#include <utility>

namespace x::Network {

NetworkSourceDescriptor::NetworkSourceDescriptor(SchemaPtr schema,
                                                 xPartition xPartition,
                                                 NodeLocation nodeLocation,
                                                 std::chrono::milliseconds waitTime,
                                                 uint32_t retryTimes)
    : SourceDescriptor(std::move(schema)), xPartition(xPartition), nodeLocation(nodeLocation), waitTime(waitTime),
      retryTimes(retryTimes) {}

SourceDescriptorPtr NetworkSourceDescriptor::create(SchemaPtr schema,
                                                    xPartition xPartition,
                                                    NodeLocation nodeLocation,
                                                    std::chrono::milliseconds waitTime,
                                                    uint32_t retryTimes) {
    return std::make_shared<NetworkSourceDescriptor>(
        NetworkSourceDescriptor(std::move(schema), xPartition, nodeLocation, waitTime, retryTimes));
}

bool NetworkSourceDescriptor::equal(SourceDescriptorPtr const& other) const {
    if (!other->instanceOf<NetworkSourceDescriptor>()) {
        return false;
    }
    auto otherNetworkSource = other->as<NetworkSourceDescriptor>();
    return schema->equals(otherNetworkSource->schema) && xPartition == otherNetworkSource->xPartition;
}

std::string NetworkSourceDescriptor::toString() const {
    return "NetworkSourceDescriptor{" + nodeLocation.createZmqURI() + " " + xPartition.toString() + "}";
}

xPartition NetworkSourceDescriptor::getxPartition() const { return xPartition; }

NodeLocation NetworkSourceDescriptor::getNodeLocation() const { return nodeLocation; }

std::chrono::milliseconds NetworkSourceDescriptor::getWaitTime() const { return waitTime; }

uint8_t NetworkSourceDescriptor::getRetryTimes() const { return retryTimes; }

SourceDescriptorPtr NetworkSourceDescriptor::copy() {
    auto copy = NetworkSourceDescriptor::create(schema->copy(), xPartition, nodeLocation, waitTime, retryTimes);
    return copy;
}

}// namespace x::Network