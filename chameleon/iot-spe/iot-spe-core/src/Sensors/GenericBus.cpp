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

#include <Sensors/GenericBus.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/magicenum/magic_enum.hpp>

namespace x::Sensors {

GenericBus::GenericBus(const char* filename, BusType type) : fileName(filename), busType(type) {
    x_INFO("Sensor Bus: Initializing {} bus at {}", magic_enum::enum_name(type), filename);
}

GenericBus::~GenericBus() { x_DEBUG("Sensor Bus: Destroying {} bus at {}", magic_enum::enum_name(this->busType), fileName); }

bool GenericBus::init(int address) { return this->initBus(address); }

bool GenericBus::write(int addr, int size, unsigned char* buffer) { return this->writeData(addr, size, buffer); }

bool GenericBus::read(int addr, int size, unsigned char* buffer) { return this->readData(addr, size, buffer); }

BusType GenericBus::getType() { return this->busType; }

}// namespace x::Sensors