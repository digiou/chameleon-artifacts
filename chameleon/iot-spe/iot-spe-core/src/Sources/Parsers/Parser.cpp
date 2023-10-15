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
#include <Common/PhysicalTypes/BasicPhysicalType.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Common/PhysicalTypes/PhysicalType.hpp>
#include <Runtime/FixedSizeBufferPool.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/QueryManager.hpp>
#include <Sources/Parsers/Parser.hpp>
#include <Util/Common.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <cstring>
#include <string>
#include <utility>

namespace x {

Parser::Parser(std::vector<PhysicalTypePtr> physicalTypes) : physicalTypes(std::move(physicalTypes)) {}

void Parser::writeFieldValueToTupleBuffer(std::string inputString,
                                          uint64_t schemaFieldIndex,
                                          Runtime::MemoryLayouts::DynamicTupleBuffer& tupleBuffer,
                                          const SchemaPtr& schema,
                                          uint64_t tupleCount,
                                          const Runtime::BufferManagerPtr& bufferManager) {
    auto fields = schema->fields;
    auto dataType = fields[schemaFieldIndex]->getDataType();
    auto physicalType = DefaultPhysicalTypeFactory().getPhysicalType(dataType);

    if (inputString.empty()) {
        throw Exceptions::RuntimeException("Input string for parsing is empty");
    }
    // TODO replace with csv parsing library
    try {
        if (physicalType->isBasicType()) {
            auto basicPhysicalType = std::dynamic_pointer_cast<BasicPhysicalType>(physicalType);
            switch (basicPhysicalType->nativeType) {
                case x::BasicPhysicalType::NativeType::INT_8: {
                    auto value = static_cast<int8_t>(std::stoi(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<int8_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::INT_16: {
                    auto value = static_cast<int16_t>(std::stol(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<int16_t>(value);

                    break;
                }
                case x::BasicPhysicalType::NativeType::INT_32: {
                    auto value = static_cast<int32_t>(std::stol(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<int32_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::INT_64: {
                    auto value = static_cast<int64_t>(std::stoll(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<int64_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::UINT_8: {
                    auto value = static_cast<uint8_t>(std::stoi(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<uint8_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::UINT_16: {
                    auto value = static_cast<uint16_t>(std::stoul(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<uint16_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::UINT_32: {
                    auto value = static_cast<uint32_t>(std::stoul(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<uint32_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::UINT_64: {
                    auto value = static_cast<uint64_t>(std::stoull(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<uint64_t>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::FLOAT: {
                    x::Util::findAndReplaceAll(inputString, ",", ".");
                    auto value = static_cast<float>(std::stof(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<float>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::DOUBLE: {
                    auto value = static_cast<double>(std::stod(inputString));
                    tupleBuffer[tupleCount][schemaFieldIndex].write<double>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::CHAR: {
                    //verify that only a single char was transmitted
                    if (inputString.size() > 1) {
                        x_FATAL_ERROR("SourceFormatIterator::mqttMessageToxBuffer: Received non char Value for CHAR Field {}",
                                        inputString);
                        throw std::invalid_argument("Value " + inputString + " is not a char");
                    }
                    char value = inputString.at(0);
                    tupleBuffer[tupleCount][schemaFieldIndex].write<char>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::TEXT: {
                    x_TRACE("Parser::writeFieldValueToTupleBuffer(): trying to write the variable length input string: {}"
                              "to tuple buffer",
                              inputString);

                    auto sizeOfInputField = inputString.size();
                    auto totalSize = sizeOfInputField + sizeof(uint32_t);
                    auto childTupleBuffer = allocateVariableLengthField(bufferManager, totalSize);

                    x_ASSERT(
                        childTupleBuffer.getBufferSize() >= totalSize,
                        "Parser::writeFieldValueToTupleBuffer(): Could not write TEXT field to tuple buffer, there was not "
                        "sufficient space available. Required space: "
                            << totalSize << ", available space: " << childTupleBuffer.getBufferSize());

                    // write out the length and the variable-sized text to the child buffer
                    (*childTupleBuffer.getBuffer<uint32_t>()) = sizeOfInputField;
                    std::memcpy(childTupleBuffer.getBuffer() + sizeof(uint32_t), inputString.c_str(), sizeOfInputField);

                    // attach the child buffer to the parent buffer and write the child buffer index in the
                    // schema field index of the tuple buffer
                    auto childIdx = tupleBuffer.getBuffer().storeChildBuffer(childTupleBuffer);
                    tupleBuffer[tupleCount][schemaFieldIndex].write<Runtime::TupleBuffer::xtedTupleBufferKey>(childIdx);

                    break;
                }
                case x::BasicPhysicalType::NativeType::BOOLEAN: {
                    //verify that a valid bool was transmitted (valid{true,false,0,1})
                    bool value = !strcasecmp(inputString.c_str(), "true") || !strcasecmp(inputString.c_str(), "1");
                    if (!value) {
                        if (strcasecmp(inputString.c_str(), "false") && strcasecmp(inputString.c_str(), "0")) {
                            x_FATAL_ERROR(
                                "Parser::writeFieldValueToTupleBuffer: Received non boolean value for BOOLEAN field: {}",
                                inputString.c_str());
                            throw std::invalid_argument("Value " + inputString + " is not a boolean");
                        }
                    }
                    tupleBuffer[tupleCount][schemaFieldIndex].write<bool>(value);
                    break;
                }
                case x::BasicPhysicalType::NativeType::UNDEFINED:
                    x_FATAL_ERROR("Parser::writeFieldValueToTupleBuffer: Field Type UNDEFINED");
            }
        } else {// char array(string) case
            // obtain pointer from buffer to fill with content via strcpy
            char* value = tupleBuffer[tupleCount][schemaFieldIndex].read<char*>();
            // remove quotation marks from start and end of value (ASSUMES QUOTATIONMARKS AROUND STRINGS)
            // improve behavior with json library
            strcpy(value, inputString.c_str());
        }
    } catch (const std::exception& e) {
        x_ERROR("Failed to convert inputString to desired x data type. Error: {}", e.what());
    }
}

}//namespace x