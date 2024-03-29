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

#ifndef x_DATA_TYPES_INCLUDE_COMMON_DATATYPES_TEXT_HPP_
#define x_DATA_TYPES_INCLUDE_COMMON_DATATYPES_TEXT_HPP_

#include <Common/DataTypes/DataType.hpp>
namespace x {

/**
 * @brief The Text type represents a variable-sized text field.
 */
class Text final : public DataType {
  public:
    virtual ~Text() = default;

    /**
    * @brief Checks if this data type is Boolean.
    */
    [[nodiscard]] bool isText() const final { return true; }

    /**
     * @brief Checks if two data types are equal.
     * @param otherDataType
     * @return
     */
    bool isEquals(DataTypePtr otherDataType) final;

    /**
     * @brief Calculates the joined data type between this data type and the other.
     * If they have no possible joined data type, the coined type is Undefined.
     * @param other data type
     * @return DataTypePtr joined data type
     */
    DataTypePtr join(DataTypePtr otherDataType) final;

    /**
     * @brief Returns a string representation of the data type.
     * @return string
     */
    std::string toString() final;
};

}// namespace x
#endif// x_DATA_TYPES_INCLUDE_COMMON_DATATYPES_TEXT_HPP_
