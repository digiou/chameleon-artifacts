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

#ifndef x_RUNTIME_INCLUDE_NAUTILUS_IR_OPERATIONS_CONSTINTOPERATION_HPP_
#define x_RUNTIME_INCLUDE_NAUTILUS_IR_OPERATIONS_CONSTINTOPERATION_HPP_

#include <Nautilus/IR/Operations/Operation.hpp>

namespace x::Nautilus::IR::Operations {

class ConstIntOperation : public Operation {
  public:
    explicit ConstIntOperation(OperationIdentifier identifier, int64_t constantValue, Types::StampPtr stamp);
    ~ConstIntOperation() override = default;
    int64_t getValue();

    template<class T>
    T getIntegerViaType();

    std::string toString() override;
    static bool classof(const Operation* Op);

  private:
    int64_t constantValue;// Can also hold uInts
};

}// namespace x::Nautilus::IR::Operations
#endif// x_RUNTIME_INCLUDE_NAUTILUS_IR_OPERATIONS_CONSTINTOPERATION_HPP_
