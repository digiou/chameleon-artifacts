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
#include <Nautilus/IR/Types/IntegerStamp.hpp>

namespace x::Nautilus::IR::Types {

IntegerStamp::IntegerStamp(BitWidth bitWidth, SignedxsSemantics signedxs)
    : Stamp(&type), bitWidth(bitWidth), signedxs(signedxs) {}

IntegerStamp::SignedxsSemantics IntegerStamp::getSignedxs() const { return signedxs; }

bool IntegerStamp::isSigned() const { return getSignedxs() == SignedxsSemantics::Signed; }

bool IntegerStamp::isUnsigned() const { return getSignedxs() == SignedxsSemantics::Unsigned; }

IntegerStamp::BitWidth IntegerStamp::getBitWidth() const { return bitWidth; }

uint32_t IntegerStamp::getNumberOfBits() const {
    switch (getBitWidth()) {
        case BitWidth::I8: return 8;
        case BitWidth::I16: return 16;
        case BitWidth::I32: return 32;
        case BitWidth::I64: return 64;
    }
}

const std::string IntegerStamp::toString() const {
    auto prefix = isUnsigned() ? "ui" : "i";
    return prefix + std::to_string(getNumberOfBits());
}

}// namespace x::Nautilus::IR::Types