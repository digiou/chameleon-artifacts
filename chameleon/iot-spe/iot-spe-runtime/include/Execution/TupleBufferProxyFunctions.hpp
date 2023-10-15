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
#include <Runtime/TupleBuffer.hpp>
namespace x::Runtime::ProxyFunctions {
void* x__Runtime__TupleBuffer__getBuffer(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getBuffer();
};
uint64_t x__Runtime__TupleBuffer__getBufferSize(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getBufferSize();
};
uint64_t x__Runtime__TupleBuffer__getNumberOfTuples(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getNumberOfTuples();
};
extern "C" __attribute__((always_inline)) void x__Runtime__TupleBuffer__setNumberOfTuples(void* thisPtr,
                                                                                            uint64_t numberOfTuples) {
    x::Runtime::TupleBuffer* tupleBuffer = static_cast<x::Runtime::TupleBuffer*>(thisPtr);
    tupleBuffer->setNumberOfTuples(numberOfTuples);
}
uint64_t x__Runtime__TupleBuffer__getOriginId(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getOriginId();
};
void x__Runtime__TupleBuffer__setOriginId(void* thisPtr, uint64_t value) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    thisPtr_->setOriginId(value);
};

uint64_t x__Runtime__TupleBuffer__getWatermark(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getWatermark();
};
void x__Runtime__TupleBuffer__setWatermark(void* thisPtr, uint64_t value) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    thisPtr_->setWatermark(value);
};
uint64_t x__Runtime__TupleBuffer__getCreationTimestampInMS(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getCreationTimestampInMS();
};
void x__Runtime__TupleBuffer__setSequenceNumber(void* thisPtr, uint64_t sequenceNumber) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->setSequenceNumber(sequenceNumber);
};
uint64_t x__Runtime__TupleBuffer__getSequenceNumber(void* thisPtr) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->getSequenceNumber();
}
void x__Runtime__TupleBuffer__setCreationTimestampInMS(void* thisPtr, uint64_t value) {
    auto* thisPtr_ = (x::Runtime::TupleBuffer*) thisPtr;
    return thisPtr_->setCreationTimestampInMS(value);
}

}// namespace x::Runtime::ProxyFunctions