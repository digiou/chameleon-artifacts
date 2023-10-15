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
#include <Execution/Operators/Streaming/Join/StreamHashJoin/StreamHashJoinOperatorHandler.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/Joining/Streaming/PhysicalxtedLoopJoinProbeOperator.hpp>
#include <utility>

namespace x::QueryCompilation::PhysicalOperators {

PhysicalxtedLoopJoinProbeOperator::PhysicalxtedLoopJoinProbeOperator(
    OperatorId id,
    SchemaPtr leftSchema,
    SchemaPtr rightSchema,
    SchemaPtr outputSchema,
    const std::string& joinFieldNameLeft,
    const std::string& joinFieldNameRight,
    const std::string& windowStartFieldName,
    const std::string& windowEndFieldName,
    const std::string& windowKeyFieldName,
    Runtime::Execution::Operators::NLJOperatorHandlerPtr operatorHandler)
    : OperatorNode(id), PhysicalxtedLoopJoinOperator(std::move(operatorHandler)),
      PhysicalBinaryOperator(id, std::move(leftSchema), std::move(rightSchema), std::move(outputSchema)),
      joinFieldNameLeft(joinFieldNameLeft), joinFieldNameRight(joinFieldNameRight), windowStartFieldName(windowStartFieldName),
      windowEndFieldName(windowEndFieldName), windowKeyFieldName(windowKeyFieldName) {}

std::string PhysicalxtedLoopJoinProbeOperator::toString() const { return "PhysicalxtedLoopJoinProbeOperator"; }

OperatorNodePtr PhysicalxtedLoopJoinProbeOperator::copy() {
    return create(id,
                  leftInputSchema,
                  rightInputSchema,
                  outputSchema,
                  joinFieldNameLeft,
                  joinFieldNameRight,
                  windowStartFieldName,
                  windowEndFieldName,
                  windowKeyFieldName,
                  operatorHandler);
}

PhysicalOperatorPtr
PhysicalxtedLoopJoinProbeOperator::create(const SchemaPtr& leftSchema,
                                            const SchemaPtr& rightSchema,
                                            const SchemaPtr& outputSchema,
                                            const std::string& joinFieldNameLeft,
                                            const std::string& joinFieldNameRight,
                                            const std::string& windowStartFieldName,
                                            const std::string& windowEndFieldName,
                                            const std::string& windowKeyFieldName,
                                            const Runtime::Execution::Operators::NLJOperatorHandlerPtr& operatorHandler) {
    return create(Util::getNextOperatorId(),
                  leftSchema,
                  rightSchema,
                  outputSchema,
                  joinFieldNameLeft,
                  joinFieldNameRight,
                  windowStartFieldName,
                  windowEndFieldName,
                  windowKeyFieldName,
                  operatorHandler);
}

PhysicalOperatorPtr
PhysicalxtedLoopJoinProbeOperator::create(const OperatorId id,
                                            const SchemaPtr& leftSchema,
                                            const SchemaPtr& rightSchema,
                                            const SchemaPtr& outputSchema,
                                            const std::string& joinFieldNameLeft,
                                            const std::string& joinFieldNameRight,
                                            const std::string& windowStartFieldName,
                                            const std::string& windowEndFieldName,
                                            const std::string& windowKeyFieldName,
                                            const Runtime::Execution::Operators::NLJOperatorHandlerPtr& operatorHandler) {
    return std::make_shared<PhysicalxtedLoopJoinProbeOperator>(id,
                                                                 leftSchema,
                                                                 rightSchema,
                                                                 outputSchema,
                                                                 joinFieldNameLeft,
                                                                 joinFieldNameRight,
                                                                 windowStartFieldName,
                                                                 windowEndFieldName,
                                                                 windowKeyFieldName,
                                                                 operatorHandler);
}

const std::string& PhysicalxtedLoopJoinProbeOperator::getJoinFieldNameLeft() const { return joinFieldNameLeft; }
const std::string& PhysicalxtedLoopJoinProbeOperator::getJoinFieldNameRight() const { return joinFieldNameRight; }
const std::string& PhysicalxtedLoopJoinProbeOperator::getWindowStartFieldName() const { return windowStartFieldName; }
const std::string& PhysicalxtedLoopJoinProbeOperator::getWindowEndFieldName() const { return windowEndFieldName; }
const std::string& PhysicalxtedLoopJoinProbeOperator::getWindowKeyFieldName() const { return windowKeyFieldName; }

}// namespace x::QueryCompilation::PhysicalOperators