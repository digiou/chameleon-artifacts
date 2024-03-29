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

#ifndef x_CORE_INCLUDE_WINDOWING_WINDOWAGGREGATIONS_COUNTAGGREGATIONDESCRIPTOR_HPP_
#define x_CORE_INCLUDE_WINDOWING_WINDOWAGGREGATIONS_COUNTAGGREGATIONDESCRIPTOR_HPP_

#include <Windowing/WindowAggregations/WindowAggregationDescriptor.hpp>
namespace x::Windowing {

/**
 * @brief
 * The CountAggregationDescriptor aggregation calculates the CountAggregationDescriptor over the window.
 */
class CountAggregationDescriptor : public WindowAggregationDescriptor {
  public:
    /**
    * Factory method to creates a CountAggregationDescriptor aggregation on a particular field.
    */
    static WindowAggregationPtr on();

    static WindowAggregationPtr create(FieldAccessExpressionNodePtr onField, FieldAccessExpressionNodePtr asField);

    DataTypePtr getInputStamp() override;
    DataTypePtr getPartialAggregateStamp() override;
    DataTypePtr getFinalAggregateStamp() override;
    /**
     * @brief Infers the stamp of the expression given the current schema and the typeInferencePhaseContext.
     * @param typeInferencePhaseContext
     * @param schema
     */
    void inferStamp(const Optimizer::TypeInferencePhaseContext& typeInferencePhaseContext, SchemaPtr schema) override;

    WindowAggregationPtr copy() override;

    virtual ~CountAggregationDescriptor() = default;

  private:
    explicit CountAggregationDescriptor(FieldAccessExpressionNodePtr onField);
    CountAggregationDescriptor(ExpressionNodePtr onField, ExpressionNodePtr asField);
};
}// namespace x::Windowing
#endif// x_CORE_INCLUDE_WINDOWING_WINDOWAGGREGATIONS_COUNTAGGREGATIONDESCRIPTOR_HPP_
