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

#include <Nautilus/Interface/DataTypes/Any.hpp>

#ifndef x_RUNTIME_INCLUDE_NAUTILUS_INTERFACE_DATATYPES_BASETYPEDREF_HPP_
#define x_RUNTIME_INCLUDE_NAUTILUS_INTERFACE_DATATYPES_BASETYPEDREF_HPP_
namespace x::Nautilus {

class BaseTypedRef : public Any {
  public:
    BaseTypedRef(const TypeIdentifier* identifier) : Any(identifier){};
};
}// namespace x::Nautilus
#endif// x_RUNTIME_INCLUDE_NAUTILUS_INTERFACE_DATATYPES_BASETYPEDREF_HPP_
