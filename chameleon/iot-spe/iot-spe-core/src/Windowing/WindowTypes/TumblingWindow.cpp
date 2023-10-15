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
#include <Util/Logger/Logger.hpp>
#include <Windowing/Runtime/WindowState.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/WindowTypes/TumblingWindow.hpp>
#include <utility>
#include <vector>

namespace x::Windowing {

TumblingWindow::TumblingWindow(TimeCharacteristicPtr timeCharacteristic, TimeMeasure size)
    : TimeBasedWindowType(std::move(timeCharacteristic)), size(std::move(size)) {}

WindowTypePtr TumblingWindow::of(TimeCharacteristicPtr timeCharacteristic, TimeMeasure size) {
    return std::dynamic_pointer_cast<WindowType>(
        std::make_shared<TumblingWindow>(TumblingWindow(std::move(timeCharacteristic), std::move(size))));
}

uint64_t TumblingWindow::calculateNextWindowEnd(uint64_t currentTs) const {
    return currentTs + size.getTime() - (currentTs % size.getTime());
}

void TumblingWindow::triggerWindows(std::vector<WindowState>& windows, uint64_t lastWatermark, uint64_t currentWatermark) const {
    x_TRACE("TumblingWindow::triggerWindows windows before={}", windows.size());
    //lastStart = last window that starts before the watermark
    long lastStart = lastWatermark - ((lastWatermark + size.getTime()) % size.getTime());
    x_TRACE("TumblingWindow::triggerWindows= lastStart={} size.getTime()={} lastWatermark={} currentWatermark={}",
              lastStart,
              size.getTime(),
              lastWatermark,
              currentWatermark);
    for (long windowStart = lastStart; windowStart + size.getTime() <= currentWatermark; windowStart += size.getTime()) {
        x_TRACE("TumblingWindow::triggerWindows  add window start ={} end={}", windowStart, windowStart + size.getTime());
        windows.emplace_back(windowStart, windowStart + size.getTime());
    }
    x_TRACE("TumblingWindow::triggerWindows windows after={}", windows.size());
}

TimeBasedWindowType::TimeBasedSubWindowType TumblingWindow::getTimeBasedSubWindowType() { return TUMBLINGWINDOW; }

TimeMeasure TumblingWindow::getSize() { return size; }

TimeMeasure TumblingWindow::getSlide() { return getSize(); }

std::string TumblingWindow::toString() {
    std::stringstream ss;
    ss << "TumblingWindow: size=" << size.getTime();
    ss << " timeCharacteristic=" << timeCharacteristic->toString();
    ss << std::endl;
    return ss.str();
}

bool TumblingWindow::equal(WindowTypePtr otherWindowType) {
    if (auto otherTumblingWindow = std::dynamic_pointer_cast<TumblingWindow>(otherWindowType)) {
        return this->size.equals(otherTumblingWindow->size)
            && this->timeCharacteristic->equals(*otherTumblingWindow->timeCharacteristic);
    }
    return false;
}
}// namespace x::Windowing