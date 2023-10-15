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
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalMultiplexOperator.hpp>
#include <utility>
namespace x::QueryCompilation::PhysicalOperators {

PhysicalOperatorPtr PhysicalMultiplexOperator::create(OperatorId id, const SchemaPtr& schema) {
    return std::make_shared<PhysicalMultiplexOperator>(id, schema);
}

PhysicalOperatorPtr PhysicalMultiplexOperator::create(SchemaPtr schema) {
    return create(Util::getNextOperatorId(), std::move(schema));
}

PhysicalMultiplexOperator::PhysicalMultiplexOperator(OperatorId id, const SchemaPtr& schema)
    : OperatorNode(id), PhysicalOperator(id), ExchangeOperatorNode(id) {
    ExchangeOperatorNode::setInputSchema(schema);
    ExchangeOperatorNode::setOutputSchema(schema);
}

std::string PhysicalMultiplexOperator::toString() const { return "PhysicalMultiplexOperator"; }
OperatorNodePtr PhysicalMultiplexOperator::copy() { return create(id, inputSchema); }

}// namespace x::QueryCompilation::PhysicalOperators