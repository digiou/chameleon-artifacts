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

#include <QueryCompiler/Operators/PhysicalOperators/Windowing/ContentBasedWindow/PhysicalThresholdWindowOperator.hpp>
#include <memory>
#include <utility>

namespace x::QueryCompilation::PhysicalOperators {

PhysicalThresholdWindowOperator::PhysicalThresholdWindowOperator(OperatorId id,
                                                                 SchemaPtr inputSchema,
                                                                 SchemaPtr outputSchema,
                                                                 Windowing::WindowOperatorHandlerPtr operatorHandler)
    : OperatorNode(id), PhysicalUnaryOperator(id, std::move(inputSchema), std::move(outputSchema)),
      operatorHandler(std::move(operatorHandler)) {}
std::shared_ptr<PhysicalThresholdWindowOperator>
PhysicalThresholdWindowOperator::create(SchemaPtr inputSchema,
                                        SchemaPtr outputSchema,
                                        Windowing::WindowOperatorHandlerPtr operatorHandler) {

    return std::make_shared<PhysicalThresholdWindowOperator>(Util::getNextOperatorId(),
                                                             inputSchema,
                                                             outputSchema,
                                                             operatorHandler);
}

std::string PhysicalThresholdWindowOperator::toString() const { return "PhysicalThresholdWindowOperator"; }

OperatorNodePtr PhysicalThresholdWindowOperator::copy() { return create(inputSchema, outputSchema, operatorHandler); }

Windowing::WindowOperatorHandlerPtr PhysicalThresholdWindowOperator::getOperatorHandler() { return operatorHandler; }
}//namespace x::QueryCompilation::PhysicalOperators