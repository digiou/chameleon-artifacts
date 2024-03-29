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
#include <Util/Experimental/SpatialType.hpp>
#include <Util/Experimental/SpatialTypeUtility.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Spatial::Util {

Experimental::SpatialType SpatialTypeUtility::stringToNodeType(const std::string spatialTypeString) {
    if (spatialTypeString == "NO_LOCATION") {
        return Experimental::SpatialType::NO_LOCATION;
    } else if (spatialTypeString == "FIXED_LOCATION") {
        return Experimental::SpatialType::FIXED_LOCATION;
    } else if (spatialTypeString == "MOBILE_NODE") {
        return Experimental::SpatialType::MOBILE_NODE;
    }
    return Experimental::SpatialType::INVALID;
}

Experimental::SpatialType SpatialTypeUtility::protobufEnumToNodeType(x::Spatial::Protobuf::SpatialType spatialType) {
    switch (spatialType) {
        case x::Spatial::Protobuf::SpatialType::NO_LOCATION: return Experimental::SpatialType::NO_LOCATION;
        case x::Spatial::Protobuf::SpatialType::FIXED_LOCATION: return Experimental::SpatialType::FIXED_LOCATION;
        case x::Spatial::Protobuf::SpatialType::MOBILE_NODE: return Experimental::SpatialType::MOBILE_NODE;
        case x::Spatial::Protobuf::SpatialType_INT_MIN_SENTINEL_DO_NOT_USE_: return Experimental::SpatialType::INVALID;
        case x::Spatial::Protobuf::SpatialType_INT_MAX_SENTINEL_DO_NOT_USE_: return Experimental::SpatialType::INVALID;
    }
    return Experimental::SpatialType::INVALID;
}

std::string SpatialTypeUtility::toString(const Experimental::SpatialType spatialType) {
    switch (spatialType) {
        case Experimental::SpatialType::NO_LOCATION: return "NO_LOCATION";
        case Experimental::SpatialType::FIXED_LOCATION: return "FIXED_LOCATION";
        case Experimental::SpatialType::MOBILE_NODE: return "MOBILE_NODE";
        case Experimental::SpatialType::INVALID: return "INVALID";
    }
}

x::Spatial::Protobuf::SpatialType SpatialTypeUtility::toProtobufEnum(Experimental::SpatialType spatialType) {
    switch (spatialType) {
        case Experimental::SpatialType::NO_LOCATION: return x::Spatial::Protobuf::SpatialType::NO_LOCATION;
        case Experimental::SpatialType::FIXED_LOCATION: return x::Spatial::Protobuf::SpatialType::FIXED_LOCATION;
        case Experimental::SpatialType::MOBILE_NODE: return x::Spatial::Protobuf::SpatialType::MOBILE_NODE;
        case Experimental::SpatialType::INVALID:
            x_FATAL_ERROR("cannot construct protobuf enum from invalid spatial type, exiting");
            exit(EXIT_FAILURE);
    }
}
}// namespace x::Spatial::Util