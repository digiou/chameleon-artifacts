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

#ifndef x_COMMON_INCLUDE_CONFIGURATIONS_CONFIGURATIONEXCEPTION_HPP_
#define x_COMMON_INCLUDE_CONFIGURATIONS_CONFIGURATIONEXCEPTION_HPP_

#include <Exceptions/RuntimeException.hpp>
#include <string>
namespace x::Configurations {

/**
 * @brief This exception is thrown when an error occurs during configuration processing.
 */
class ConfigurationException : public Exceptions::RuntimeException {
  public:
    /**
     * @brief Construct a configuration exception from a message and include the current stack trace.
     * @param message The exception message.
     */
    explicit ConfigurationException(const std::string& message,
                                    std::string&& stacktrace = collectAndPrintStacktrace(),
                                    std::source_location location = std::source_location::current());
};

}// namespace x::Configurations
#endif// x_COMMON_INCLUDE_CONFIGURATIONS_CONFIGURATIONEXCEPTION_HPP_
