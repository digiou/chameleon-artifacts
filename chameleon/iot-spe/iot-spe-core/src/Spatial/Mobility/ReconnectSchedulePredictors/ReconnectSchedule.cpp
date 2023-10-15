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
#include <Spatial/Mobility/ReconnectSchedulePredictors/ReconnectSchedule.hpp>
#include <utility>

namespace x::Spatial::Mobility::Experimental {

ReconnectSchedule::ReconnectSchedule(std::vector<ReconnectPoint> reconnectVector) : reconnectVector(std::move(reconnectVector)) {}

const std::vector<ReconnectPoint>& ReconnectSchedule::getReconnectVector() const { return reconnectVector; }

ReconnectSchedule ReconnectSchedule::Empty() { return ReconnectSchedule({}); }

bool ReconnectSchedule::removeNextReconnect() {
    if (reconnectVector.empty()) {
        return false;
    }
    reconnectVector.erase(reconnectVector.begin());
    return true;
}

}// namespace x::Spatial::Mobility::Experimental