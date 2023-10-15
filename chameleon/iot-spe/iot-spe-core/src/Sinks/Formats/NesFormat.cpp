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

#include "SerializableOperator.pb.h"
#include <GRPC/Serialization/SchemaSerializationUtil.hpp>
#include <Runtime/BufferManager.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Sinks/Formats/xFormat.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <iostream>
#include <utility>

namespace x {

xFormat::xFormat(SchemaPtr schema, Runtime::BufferManagerPtr bufferManager)
    : SinkFormat(std::move(schema), std::move(bufferManager)) {
    serializedSchema = std::make_shared<SerializableSchema>();
}

std::string xFormat::getFormattedBuffer(Runtime::TupleBuffer& inputBuffer) {
    std::string out((char*) inputBuffer.getBuffer(), inputBuffer.getNumberOfTuples() * getSchemaPtr()->getSchemaSizeInBytes());
    return out;
}

std::string xFormat::toString() { return "x_FORMAT"; }

FormatTypes xFormat::getSinkFormat() { return FormatTypes::x_FORMAT; }

FormatIterator xFormat::getTupleIterator(Runtime::TupleBuffer&) { x_NOT_IMPLEMENTED(); }

std::string xFormat::getFormattedSchema() {
    SerializableSchemaPtr protoBuff = SchemaSerializationUtil::serializeSchema(schema, serializedSchema.get());
    return protoBuff->SerializeAsString();
}

}// namespace x