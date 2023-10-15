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

#ifndef x_RUNTIME_INCLUDE_RUNTIME_MEMORYLAYOUT_BUFFERACCESSEXCEPTION_HPP_
#define x_RUNTIME_INCLUDE_RUNTIME_MEMORYLAYOUT_BUFFERACCESSEXCEPTION_HPP_

#include <Exceptions/RuntimeException.hpp>

#include <string>

namespace x {

/**
 * @brief This exception is thrown when an error occurs during UDF processing.
 */
class BufferAccessException : public Exceptions::RuntimeException {
  public:
    /**
     * @brief Construct a UDF exception from a message and include the current stack trace.
     * @param message The exception message.
     */
    explicit BufferAccessException(const std::string& message);

  private:
    const std::string message;
};

}// namespace x
#endif// x_RUNTIME_INCLUDE_RUNTIME_MEMORYLAYOUT_BUFFERACCESSEXCEPTION_HPP_