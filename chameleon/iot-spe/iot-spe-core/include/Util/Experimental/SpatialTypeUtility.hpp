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

#ifndef x_CORE_INCLUDE_UTIL_EXPERIMENTAL_SPATIALTYPEUTILITY_HPP_
#define x_CORE_INCLUDE_UTIL_EXPERIMENTAL_SPATIALTYPEUTILITY_HPP_

#include <Util/Experimental/SpatialType.hpp>
#include <WorkerLocation.grpc.pb.h>

namespace x::Spatial::Util {

/**
 * @brief this class contains functions to convert a spatial type enum to its equivalent protobuf type and vice versa
 * as well as functions to convert the node type enum to/from string
 */
class SpatialTypeUtility {
  public:
    static Experimental::SpatialType stringToNodeType(const std::string spatialTypeString);

    static Experimental::SpatialType protobufEnumToNodeType(x::Spatial::Protobuf::SpatialType spatialType);

    static std::string toString(Experimental::SpatialType spatialType);

    static x::Spatial::Protobuf::SpatialType toProtobufEnum(Experimental::SpatialType spatialType);
};

}// namespace x::Spatial::Util

#endif// x_CORE_INCLUDE_UTIL_EXPERIMENTAL_SPATIALTYPEUTILITY_HPP_
