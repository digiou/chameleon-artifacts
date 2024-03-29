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

#ifndef x_CORE_INCLUDE_WINDOWING_TIMECHARACTERISTIC_HPP_
#define x_CORE_INCLUDE_WINDOWING_TIMECHARACTERISTIC_HPP_

#include <API/Windowing.hpp>
#include <Windowing/WindowMeasures/TimeUnit.hpp>
#include <Windowing/WindowingForwardRefs.hpp>
namespace x::Windowing {

/**
 * @brief The time stamp characteristic represents if an window is in event or processing time.
 */
class TimeCharacteristic final {
  public:
    constexpr static const auto RECORD_CREATION_TS_FIELD_NAME = "$record.creationTs";
    /**
     * @brief The type as enum.
     */
    enum class Type : uint8_t { IngestionTime, EventTime };
    explicit TimeCharacteristic(Type type);
    TimeCharacteristic(Type type, AttributeFieldPtr field, TimeUnit unit);

    /**
     * @brief Factory to create a time characteristic for ingestion time window
     * @return TimeCharacteristicPtr
     */
    static TimeCharacteristicPtr createIngestionTime();

    /**
     * @brief Factory to create a event time window with an time extractor on a specific field.
     * @param field the field from which we want to extract the time.
     * @return
     */
    static TimeCharacteristicPtr createEventTime(ExpressionItem field, const TimeUnit& unit = API::Milliseconds());

    /**
     * @return The TimeCharacteristic type.
     */
    Type getType();

    /**
     * @return  If it is a event time window this returns the field, from which we extract the time stamp.
     */
    AttributeFieldPtr getField();

    /**
     * @brief Set the field from which we extract the time stamp.
     * @param field for extracting the time stamp
     */
    void setField(AttributeFieldPtr field);

    /**
     * @brief Compares for equality
     * @param other: Object that we want to compare this to
     * @return Boolean
     */
    virtual bool equals(const TimeCharacteristic& other) const;

    std::string toString();
    std::string getTypeAsString();
    TimeUnit getTimeUnit();

    void setTimeUnit(const TimeUnit unit);

  private:
    Type type;
    AttributeFieldPtr field;
    TimeUnit unit;
};
}// namespace x::Windowing

#endif// x_CORE_INCLUDE_WINDOWING_TIMECHARACTERISTIC_HPP_
