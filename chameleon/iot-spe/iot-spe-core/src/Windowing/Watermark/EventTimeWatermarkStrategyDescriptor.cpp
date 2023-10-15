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

#include <API/AttributeField.hpp>
#include <API/Expressions/Expressions.hpp>
#include <API/Schema.hpp>
#include <Exceptions/InvalidFieldException.hpp>
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Util/Logger/Logger.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/Watermark/EventTimeWatermarkStrategyDescriptor.hpp>
#include <sstream>
#include <utility>

namespace x::Windowing {

EventTimeWatermarkStrategyDescriptor::EventTimeWatermarkStrategyDescriptor(const ExpressionItem& onField,
                                                                           TimeMeasure allowedLatexs,
                                                                           TimeUnit unit)
    : onField(onField.getExpressionNode()), unit(std::move(unit)), allowedLatexs(std::move(allowedLatexs)) {}

WatermarkStrategyDescriptorPtr
EventTimeWatermarkStrategyDescriptor::create(const ExpressionItem& onField, TimeMeasure allowedLatexs, TimeUnit unit) {
    return std::make_shared<EventTimeWatermarkStrategyDescriptor>(
        Windowing::EventTimeWatermarkStrategyDescriptor(onField.getExpressionNode(),
                                                        std::move(allowedLatexs),
                                                        std::move(unit)));
}

ExpressionNodePtr EventTimeWatermarkStrategyDescriptor::getOnField() const { return onField; }

void EventTimeWatermarkStrategyDescriptor::setOnField(const ExpressionNodePtr& newField) { this->onField = newField; }

TimeMeasure EventTimeWatermarkStrategyDescriptor::getAllowedLatexs() const { return allowedLatexs; }

bool EventTimeWatermarkStrategyDescriptor::equal(WatermarkStrategyDescriptorPtr other) {
    auto eventTimeWatermarkStrategyDescriptor = other->as<EventTimeWatermarkStrategyDescriptor>();
    return eventTimeWatermarkStrategyDescriptor->onField->equal(onField)
        && eventTimeWatermarkStrategyDescriptor->allowedLatexs.getTime() == allowedLatexs.getTime();
}

TimeUnit EventTimeWatermarkStrategyDescriptor::getTimeUnit() const { return unit; }

void EventTimeWatermarkStrategyDescriptor::setTimeUnit(const TimeUnit& newUnit) { this->unit = newUnit; }

std::string EventTimeWatermarkStrategyDescriptor::toString() {
    std::stringstream ss;
    ss << "TYPE = EVENT-TIME,";
    ss << "FIELD =" << onField->toString() << ",";
    ss << "ALLOWED-LATExS =" << allowedLatexs.toString();
    return ss.str();
}

bool EventTimeWatermarkStrategyDescriptor::inferStamp(const Optimizer::TypeInferencePhaseContext&, SchemaPtr schema) {
    auto fieldAccessExpression = onField->as<FieldAccessExpressionNode>();
    auto fieldName = fieldAccessExpression->getFieldName();
    //Check if the field exists in the schema
    auto existingField = schema->hasFieldName(fieldName);
    if (existingField) {
        fieldAccessExpression->updateFieldName(existingField->getName());
        return true;
    } else if (fieldName == Windowing::TimeCharacteristic::RECORD_CREATION_TS_FIELD_NAME) {
        return true;
    }
    x_ERROR("EventTimeWaterMark is using a non existing field  {}", fieldName);
    throw InvalidFieldException("EventTimeWaterMark is using a non existing field " + fieldName);
}

}// namespace x::Windowing