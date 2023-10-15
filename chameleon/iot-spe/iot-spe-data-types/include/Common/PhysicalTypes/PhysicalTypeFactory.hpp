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

#ifndef x_DATA_TYPES_INCLUDE_COMMON_PHYSICALTYPES_PHYSICALTYPEFACTORY_HPP_
#define x_DATA_TYPES_INCLUDE_COMMON_PHYSICALTYPES_PHYSICALTYPEFACTORY_HPP_

#include <memory>

namespace x {

class DataType;
using DataTypePtr = std::shared_ptr<DataType>;

class PhysicalType;
using PhysicalTypePtr = std::shared_ptr<PhysicalType>;

/**
 * @brief The physical type factory converts a x data type into a physical representation.
 * This is implemented as an abstract factory, as in the future we could identify different data type mappings, depending on the underling hardware.
 */
class PhysicalTypeFactory {
  public:
    PhysicalTypeFactory() = default;

    /**
     * @brief Translates a x data type into a corresponding physical type.
     * @param dataType
     * @return PhysicalTypePtr
     */
    virtual PhysicalTypePtr getPhysicalType(DataTypePtr dataType) = 0;
};

}// namespace x

#endif// x_DATA_TYPES_INCLUDE_COMMON_PHYSICALTYPES_PHYSICALTYPEFACTORY_HPP_
