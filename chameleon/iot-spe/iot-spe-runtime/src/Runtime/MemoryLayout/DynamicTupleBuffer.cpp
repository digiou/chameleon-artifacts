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
#include <API/Schema.hpp>
#include <Common/DataTypes/DataType.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Runtime/MemoryLayout/BufferAccessException.hpp>
#include <Runtime/MemoryLayout/ColumnLayout.hpp>
#include <Runtime/MemoryLayout/DynamicTupleBuffer.hpp>
#include <Runtime/MemoryLayout/RowLayout.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>

namespace x::Runtime::MemoryLayouts {

DynamicField::DynamicField(uint8_t* address, PhysicalTypePtr physicalType) : address(address), physicalType(physicalType) {}

DynamicField DynamicTuple::operator[](std::size_t fieldIndex) {
    auto* bufferBasePointer = buffer.getBuffer<uint8_t>();
    auto offset = memoryLayout->getFieldOffset(tupleIndex, fieldIndex);
    auto* basePointer = bufferBasePointer + offset;
    auto physicalType = memoryLayout->getPhysicalTypes()[fieldIndex];
    return DynamicField{basePointer, physicalType};
}

DynamicField DynamicTuple::operator[](std::string fieldName) {
    auto fieldIndex = memoryLayout->getFieldIndexFromName(fieldName);
    if (!fieldIndex.has_value()) {
        throw BufferAccessException("field name " + fieldName + " dose not exist in layout");
    }
    return this->operator[](memoryLayout->getFieldIndexFromName(fieldName).value());
}

DynamicTuple::DynamicTuple(const uint64_t tupleIndex, MemoryLayoutPtr memoryLayout, TupleBuffer buffer)
    : tupleIndex(tupleIndex), memoryLayout(std::move(memoryLayout)), buffer(std::move(buffer)) {}

std::string DynamicTuple::toString(const SchemaPtr& schema) {
    std::stringstream ss;
    for (uint32_t i = 0; i < schema->getSize(); ++i) {
        const auto dataType = schema->get(i)->getDataType();
        DynamicField currentField = this->operator[](i);
        ss << currentField.toString() << "|";
    }
    return ss.str();
}

std::string DynamicField::toString() {
    std::stringstream ss;

    std::string currentFieldContentAsString = this->physicalType->convertRawToString(this->address);
    ss << currentFieldContentAsString;
    return ss.str();
}

bool DynamicField::equal(const DynamicField& rhs) const { return std::memcmp(address, rhs.address, physicalType->size()) == 0; }

bool DynamicField::operator==(const DynamicField& rhs) const { return equal(rhs); };

bool DynamicField::operator!=(const DynamicField& rhs) const { return !equal(rhs); }

const PhysicalTypePtr& DynamicField::getPhysicalType() const { return physicalType; };

uint64_t DynamicTupleBuffer::getCapacity() const { return memoryLayout->getCapacity(); }

uint64_t DynamicTupleBuffer::getNumberOfTuples() const { return buffer.getNumberOfTuples(); }

void DynamicTupleBuffer::setNumberOfTuples(uint64_t value) { buffer.setNumberOfTuples(value); }

DynamicTuple DynamicTupleBuffer::operator[](std::size_t tupleIndex) const {
    if (tupleIndex >= getCapacity()) {
        throw BufferAccessException("index " + std::to_string(tupleIndex) + " is out of bound for capacity"
                                    + std::to_string(getCapacity()));
    }
    return {tupleIndex, memoryLayout, buffer};
}

DynamicTupleBuffer::DynamicTupleBuffer(const MemoryLayoutPtr& memoryLayout, TupleBuffer buffer)
    : memoryLayout(memoryLayout), buffer(buffer) {
    x_ASSERT(memoryLayout->getBufferSize() == buffer.getBufferSize(),
               "Buffer size of layout has to be same then from the buffer.");
}

TupleBuffer DynamicTupleBuffer::getBuffer() { return buffer; }
std::ostream& operator<<(std::ostream& os, const DynamicTupleBuffer& buffer) {
    auto buf = buffer;
    auto str = buf.toString(buffer.memoryLayout->getSchema());
    os << str;
    return os;
}

DynamicTupleBuffer::TupleIterator DynamicTupleBuffer::begin() { return TupleIterator(*this); }
DynamicTupleBuffer::TupleIterator DynamicTupleBuffer::end() { return TupleIterator(*this, getNumberOfTuples()); }

std::string DynamicTupleBuffer::toString(const SchemaPtr& schema, bool showHeader) {
    std::stringstream str;
    std::vector<uint32_t> physicalSizes;
    std::vector<PhysicalTypePtr> types;
    auto physicalDataTypeFactory = DefaultPhysicalTypeFactory();
    for (uint32_t i = 0; i < schema->getSize(); ++i) {
        auto physicalType = physicalDataTypeFactory.getPhysicalType(schema->get(i)->getDataType());
        physicalSizes.push_back(physicalType->size());
        types.push_back(physicalType);
        x_TRACE("DynamicTupleBuffer: {} {} {} {}",
                  std::string("Field Size "),
                  schema->get(i)->toString(),
                  std::string(": "),
                  std::to_string(physicalType->size()));
    }

    if (showHeader) {
        str << "+----------------------------------------------------+" << std::endl;
        str << "|";
        for (uint32_t i = 0; i < schema->getSize(); ++i) {
            str << schema->get(i)->getName() << ":"
                << physicalDataTypeFactory.getPhysicalType(schema->get(i)->getDataType())->toString() << "|";
        }
        str << std::endl;
        str << "+----------------------------------------------------+" << std::endl;
    }

    for (auto&& it : *this) {
        str << "|";
        DynamicTuple dynamicTuple = (it);
        str << dynamicTuple.toString(schema);
        str << std::endl;
    }
    if (showHeader) {
        str << "+----------------------------------------------------+";
    }
    return str.str();
}

DynamicTupleBuffer::TupleIterator::TupleIterator(DynamicTupleBuffer& buffer) : TupleIterator(buffer, 0) {}

DynamicTupleBuffer::TupleIterator::TupleIterator(DynamicTupleBuffer& buffer, uint64_t currentIndex)
    : buffer(buffer), currentIndex(currentIndex) {}

DynamicTupleBuffer::TupleIterator& DynamicTupleBuffer::TupleIterator::operator++() {
    currentIndex++;
    return *this;
}

const DynamicTupleBuffer::TupleIterator DynamicTupleBuffer::TupleIterator::operator++(int) {
    TupleIterator retval = *this;
    ++(*this);
    return retval;
}

bool DynamicTupleBuffer::TupleIterator::operator==(TupleIterator other) const {
    return currentIndex == other.currentIndex && &buffer == &other.buffer;
}

bool DynamicTupleBuffer::TupleIterator::operator!=(TupleIterator other) const { return !(*this == other); }

DynamicTuple DynamicTupleBuffer::TupleIterator::operator*() const { return buffer[currentIndex]; }

MemoryLayoutPtr DynamicTupleBuffer::getMemoryLayout() const { return memoryLayout; }

DynamicTupleBuffer DynamicTupleBuffer::createDynamicTupleBuffer(Runtime::TupleBuffer buffer, const SchemaPtr& schema) {
    if (schema->getLayoutType() == Schema::MemoryLayoutType::ROW_LAYOUT) {
        auto memoryLayout = Runtime::MemoryLayouts::RowLayout::create(schema, buffer.getBufferSize());
        return Runtime::MemoryLayouts::DynamicTupleBuffer(memoryLayout, buffer);
    } else if (schema->getLayoutType() == Schema::MemoryLayoutType::COLUMNAR_LAYOUT) {
        auto memoryLayout = Runtime::MemoryLayouts::ColumnLayout::create(schema, buffer.getBufferSize());
        return Runtime::MemoryLayouts::DynamicTupleBuffer(memoryLayout, buffer);
    } else {
        x_NOT_IMPLEMENTED();
    }
}

}// namespace x::Runtime::MemoryLayouts