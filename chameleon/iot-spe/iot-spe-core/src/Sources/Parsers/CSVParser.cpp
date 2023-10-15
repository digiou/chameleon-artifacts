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

#include <API/Schema.hpp>
#include <Common/PhysicalTypes/BasicPhysicalType.hpp>
#include <Exceptions/RuntimeException.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Sources/Parsers/CSVParser.hpp>
#include <Util/Common.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <string>

using namespace std::string_literals;
namespace x {

CSVParser::CSVParser(uint64_t numberOfSchemaFields, std::vector<x::PhysicalTypePtr> physicalTypes, std::string delimiter)
    : Parser(physicalTypes), numberOfSchemaFields(numberOfSchemaFields), physicalTypes(std::move(physicalTypes)),
      delimiter(std::move(delimiter)) {}

bool CSVParser::writeInputTupleToTupleBuffer(const std::string& csvInputLine,
                                             uint64_t tupleCount,
                                             Runtime::MemoryLayouts::DynamicTupleBuffer& tupleBuffer,
                                             const SchemaPtr& schema,
                                             const Runtime::BufferManagerPtr& bufferManager) {
    x_TRACE("CSVParser::parseCSVLine: Current TupleCount:  {}", tupleCount);

    std::vector<std::string> values;
    try {
        values = x::Util::splitWithStringDelimiter<std::string>(csvInputLine, delimiter);
    } catch (std::exception e) {
        x_THROW_RUNTIME_ERROR(
            "CSVParser::writeInputTupleToTupleBuffer: An error occurred while splitting delimiter. ERROR: " << strerror(errno));
    }

    if (values.size() != schema->getSize()) {
        x_THROW_RUNTIME_ERROR(
            "CSVParser: The input line does not contain the right number of delimited fields. Fields in schema: "
            + std::to_string(schema->getSize()) + " Fields in line: " + std::to_string(values.size())
            + " Schema: " + schema->toString() + " Line: " + csvInputLine);
    }
    // iterate over fields of schema and cast string values to correct type
    for (uint64_t j = 0; j < numberOfSchemaFields; j++) {
        auto field = physicalTypes[j];
        x_TRACE("Current value is:  {}", values[j]);
        writeFieldValueToTupleBuffer(values[j], j, tupleBuffer, schema, tupleCount, bufferManager);
    }
    return true;
}
}// namespace x