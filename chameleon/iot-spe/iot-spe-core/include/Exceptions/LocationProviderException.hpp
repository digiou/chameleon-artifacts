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

#ifndef x_CORE_INCLUDE_EXCEPTIONS_LOCATIONPROVIDEREXCEPTION_HPP_
#define x_CORE_INCLUDE_EXCEPTIONS_LOCATIONPROVIDEREXCEPTION_HPP_

#include <exception>
#include <string>

namespace x::Spatial::Exception {

/**
 * @brief an exception indicating a failure in the location provider
 */
class LocationProviderException : public std::exception {
  public:
    explicit LocationProviderException(std::string message);

    const char* what() const noexcept override;

  private:
    std::string message;
};
}// namespace x::Spatial::Exception
#endif// x_CORE_INCLUDE_EXCEPTIONS_LOCATIONPROVIDEREXCEPTION_HPP_
