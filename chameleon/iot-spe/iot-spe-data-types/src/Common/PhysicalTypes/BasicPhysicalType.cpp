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

#include <Common/PhysicalTypes/BasicPhysicalType.hpp>
#include <Util/Logger/Logger.hpp>
#include <sstream>
#include <utility>

namespace x {

BasicPhysicalType::BasicPhysicalType(DataTypePtr type, NativeType nativeType)
    : PhysicalType(std::move(type)), nativeType(nativeType) {}

PhysicalTypePtr BasicPhysicalType::create(const DataTypePtr& type, NativeType nativeType) {
    return std::make_shared<BasicPhysicalType>(type, nativeType);
}

uint64_t BasicPhysicalType::size() const {
    switch (nativeType) {
        case NativeType::INT_8: return sizeof(int8_t);
        case NativeType::INT_16: return sizeof(int16_t);
        case NativeType::INT_32: return sizeof(int32_t);
        case NativeType::INT_64: return sizeof(int64_t);
        case NativeType::UINT_8: return sizeof(uint8_t);
        case NativeType::UINT_16: return sizeof(uint16_t);
        case NativeType::UINT_32: return sizeof(uint32_t);
        case NativeType::UINT_64: return sizeof(uint64_t);
        case NativeType::FLOAT: return sizeof(float);
        case NativeType::DOUBLE: return sizeof(double);
        case NativeType::BOOLEAN: return sizeof(bool);
        case NativeType::CHAR: return sizeof(char);
        case NativeType::TEXT: return sizeof(uint32_t);// we store the child buffer index (xtedTupleBufferKey) in the tuple
        case NativeType::UNDEFINED: return -1;
    }
    return -1;
}
std::string BasicPhysicalType::convertRawToString(void const* data) const noexcept {
    if (!data) {
        return std::string();
    }
    switch (nativeType) {
        case NativeType::INT_8: return std::to_string(*reinterpret_cast<int8_t const*>(data));
        case NativeType::UINT_8: return std::to_string(*reinterpret_cast<uint8_t const*>(data));
        case NativeType::INT_16: return std::to_string(*reinterpret_cast<int16_t const*>(data));
        case NativeType::UINT_16: return std::to_string(*reinterpret_cast<uint16_t const*>(data));
        case NativeType::INT_32: return std::to_string(*reinterpret_cast<int32_t const*>(data));
        case NativeType::UINT_32: return std::to_string(*reinterpret_cast<uint32_t const*>(data));
        case NativeType::INT_64: return std::to_string(*reinterpret_cast<int64_t const*>(data));
        case NativeType::UINT_64: return std::to_string(*reinterpret_cast<uint64_t const*>(data));
        case NativeType::FLOAT: return std::to_string(*reinterpret_cast<float const*>(data));
        case NativeType::DOUBLE: return std::to_string(*reinterpret_cast<double const*>(data));
        case NativeType::BOOLEAN: return std::to_string(*reinterpret_cast<bool const*>(data));
        case NativeType::TEXT: {
            x_DEBUG(
                "BasicPhysicalType::convertRawToString(): TEXT conversion to string not implemented. Should never be here.");
            return "TEXT conversion to string not implemented.";
        }
        case NativeType::CHAR:
            if (size() != 1) {
                return "invalid char type";
            }
            return std::string{*static_cast<char const*>(data)};
        default: return "invalid native type";
    }
}

std::string BasicPhysicalType::convertRawToStringWithoutFill(void const* data) const noexcept {

    if (!data) {
        return std::string();
    }
    switch (nativeType) {
        case NativeType::INT_8: return std::to_string(*reinterpret_cast<int8_t const*>(data));
        case NativeType::UINT_8: return std::to_string(*reinterpret_cast<uint8_t const*>(data));
        case NativeType::INT_16: return std::to_string(*reinterpret_cast<int16_t const*>(data));
        case NativeType::UINT_16: return std::to_string(*reinterpret_cast<uint16_t const*>(data));
        case NativeType::INT_32: return std::to_string(*reinterpret_cast<int32_t const*>(data));
        case NativeType::UINT_32: return std::to_string(*reinterpret_cast<uint32_t const*>(data));
        case NativeType::INT_64: return std::to_string(*reinterpret_cast<int64_t const*>(data));
        case NativeType::UINT_64: return std::to_string(*reinterpret_cast<uint64_t const*>(data));
        case NativeType::FLOAT: return std::to_string(*reinterpret_cast<float const*>(data));
        case NativeType::DOUBLE: return std::to_string(*reinterpret_cast<double const*>(data));
        case NativeType::BOOLEAN: return std::to_string(*reinterpret_cast<bool const*>(data));
        case NativeType::TEXT: {
            return "BasicPhysicalType::convertRawToString(): TEXT conversion to string not implemented. Should never be here.";
        }
        case NativeType::CHAR:
            if (size() != 1) {
                return "invalid char type";
            }
            return std::string{*static_cast<char const*>(data)};
        default: return "invalid native type";
    }
}

std::string BasicPhysicalType::toString() const noexcept {
    switch (nativeType) {
        case NativeType::INT_8: return "INT8";
        case NativeType::UINT_8: return "UINT8";
        case NativeType::INT_16: return "INT16";
        case NativeType::UINT_16: return "UINT16";
        case NativeType::INT_32: return "INT32";
        case NativeType::UINT_32: return "UINT32";
        case NativeType::INT_64: return "INT64";
        case NativeType::UINT_64: return "UINT64";
        case NativeType::FLOAT: return "FLOAT32";
        case NativeType::DOUBLE: return "FLOAT64";
        case NativeType::BOOLEAN: return "BOOLEAN";
        case NativeType::CHAR: return "CHAR";
        case NativeType::TEXT: return "TEXT";
        case NativeType::UNDEFINED: return "UNDEFINED";
    }
    return "";
}

}// namespace x