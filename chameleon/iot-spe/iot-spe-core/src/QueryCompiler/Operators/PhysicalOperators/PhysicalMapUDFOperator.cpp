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

#include <Catalogs/UDF/UDFDescriptor.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalMapUDFOperator.hpp>

namespace x::QueryCompilation::PhysicalOperators {
PhysicalMapUDFOperator::PhysicalMapUDFOperator(OperatorId id,
                                               const SchemaPtr& inputSchema,
                                               const SchemaPtr& outputSchema,
                                               const Catalogs::UDF::UDFDescriptorPtr& udfDescriptor)
    : OperatorNode(id), PhysicalUnaryOperator(id, std::move(inputSchema), std::move(outputSchema)),
      udfDescriptor(std::move(udfDescriptor)) {}

PhysicalOperatorPtr PhysicalMapUDFOperator::create(const SchemaPtr& inputSchema,
                                                   const SchemaPtr& outputSchema,
                                                   const Catalogs::UDF::UDFDescriptorPtr udfDescriptor) {
    return create(Util::getNextOperatorId(), inputSchema, outputSchema, udfDescriptor);
}

PhysicalOperatorPtr PhysicalMapUDFOperator::create(OperatorId id,
                                                   const SchemaPtr& inputSchema,
                                                   const SchemaPtr& outputSchema,
                                                   const Catalogs::UDF::UDFDescriptorPtr& udfDescriptor) {
    return std::make_shared<PhysicalMapUDFOperator>(id, inputSchema, outputSchema, udfDescriptor);
}

std::string PhysicalMapUDFOperator::toString() const { return "PhysicalMapUDFOperator"; }

OperatorNodePtr PhysicalMapUDFOperator::copy() {
    auto result = create(id, inputSchema, outputSchema, udfDescriptor);
    result->addAllProperties(properties);
    return result;
}

Catalogs::UDF::UDFDescriptorPtr PhysicalMapUDFOperator::getUDFDescriptor() { return udfDescriptor; }
}// namespace x::QueryCompilation::PhysicalOperators