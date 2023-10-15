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
#include <Exceptions/RuntimeException.hpp>
#include <Operators/AbstractOperators/OriginIdAssignmentOperator.hpp>

namespace x {

OriginIdAssignmentOperator::OriginIdAssignmentOperator(OperatorId operatorId, OriginId originId)
    : OperatorNode(operatorId), originId(originId) {}

const std::vector<OriginId> OriginIdAssignmentOperator::getOutputOriginIds() const {
    if (originId == INVALID_ORIGIN_ID) {
        throw Exceptions::RuntimeException(
            "The origin id should not be invalid. Maybe the OriginIdInference rule was not executed.");
    }
    return {originId};
}

void OriginIdAssignmentOperator::setOriginId(OriginId originId) { this->originId = originId; }

OriginId OriginIdAssignmentOperator::getOriginId() const { return originId; }

}// namespace x