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

#ifndef x_CORE_INCLUDE_WINDOWING_DISTRIBUTIONCHARACTERISTIC_HPP_
#define x_CORE_INCLUDE_WINDOWING_DISTRIBUTIONCHARACTERISTIC_HPP_
#include <memory>

namespace x::Windowing {

class DistributionCharacteristic;
using DistributionCharacteristicPtr = std::shared_ptr<DistributionCharacteristic>;

/**
 * @brief The time stamp characteristic represents if an window is in event or processing time.
 */
class DistributionCharacteristic {
  public:
    /**
     * @brief The type as enum.
     */
    enum class Type : uint8_t { Complete, Slicing, Combining, Merging };
    explicit DistributionCharacteristic(Type type);

    /**
     * @brief Factory to create central window that do slcicing and combining
     * @return DistributionCharacteristicPtr
     */
    static DistributionCharacteristicPtr createCompleteWindowType();

    /**
     * @brief Factory to to create a window slicer
     * @return
     */
    static DistributionCharacteristicPtr createSlicingWindowType();

    /**
    * @brief Factory to create a window combiner
    * @return
    */
    static DistributionCharacteristicPtr createCombiningWindowType();

    /**
    * @brief Factory to create a window merger
    * @return
    */
    static DistributionCharacteristicPtr createMergingWindowType();

    /**
     * @return The DistributionCharacteristic type.
     */
    Type getType();

    /**
   * @return The DistributionCharacteristic type.
   */
    std::string toString();

    /**
     * @brief Checks if the two are equal
     * @param other: DistributionCharacteristic that we want to check if they are equal
     * @return Boolean
     */
    bool equals(const DistributionCharacteristic& other) const;

  private:
    Type type;
};

}// namespace x::Windowing
#endif// x_CORE_INCLUDE_WINDOWING_DISTRIBUTIONCHARACTERISTIC_HPP_
