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

#include <Util/Backward/backward.hpp>
#include <Util/StacktraceLoader.hpp>

#define CALLSTACK_MAX_SIZE 32

namespace x {
/**
 * @brief This methods collects the call stacks and prints is
 */
std::string collectAndPrintStacktrace() {
    backward::StackTrace stackTrace;
    backward::Printer printer;
    stackTrace.load_here(CALLSTACK_MAX_SIZE);
    printer.object = true;
    printer.color_mode = backward::ColorMode::always;
    std::stringbuf buffer;
    std::ostream os(&buffer);
    printer.print(stackTrace, os);
    //    x_ERROR("Stacktrace:\n {}", buffer.str());
    return buffer.str();
}

void xErrorHandler() { collectAndPrintStacktrace(); }

}// namespace x