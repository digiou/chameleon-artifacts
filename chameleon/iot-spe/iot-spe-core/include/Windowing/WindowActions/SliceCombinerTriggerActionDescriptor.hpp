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

#ifndef x_CORE_INCLUDE_WINDOWING_WINDOWACTIONS_SLICECOMBINERTRIGGERACTIONDESCRIPTOR_HPP_
#define x_CORE_INCLUDE_WINDOWING_WINDOWACTIONS_SLICECOMBINERTRIGGERACTIONDESCRIPTOR_HPP_
#include <Windowing/WindowActions/BaseWindowActionDescriptor.hpp>

namespace x::Windowing {

class SliceCombinerTriggerActionDescriptor : public BaseWindowActionDescriptor {
  public:
    ~SliceCombinerTriggerActionDescriptor() noexcept override = default;
    static WindowActionDescriptorPtr create();
    ActionType getActionType() override;

  private:
    SliceCombinerTriggerActionDescriptor();
};
}// namespace x::Windowing
#endif// x_CORE_INCLUDE_WINDOWING_WINDOWACTIONS_SLICECOMBINERTRIGGERACTIONDESCRIPTOR_HPP_
