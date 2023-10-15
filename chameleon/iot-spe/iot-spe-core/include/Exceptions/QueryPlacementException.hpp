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

#ifndef x_CORE_INCLUDE_EXCEPTIONS_QUERYPLACEMENTEXCEPTION_HPP_
#define x_CORE_INCLUDE_EXCEPTIONS_QUERYPLACEMENTEXCEPTION_HPP_

#include <Common/Identifiers.hpp>
#include <Exceptions/RequestExecutionException.hpp>
#include <stdexcept>
#include <string>

namespace x::Exceptions {

/**
 * @brief Exception indicating problem during operator placement phase
 */
class QueryPlacementException : public Exceptions::RequestExecutionException {

  public:
    explicit QueryPlacementException(SharedQueryId sharedQueryId, const std::string& message);

    const char* what() const noexcept override;
};
}// namespace x::Exceptions
#endif// x_CORE_INCLUDE_EXCEPTIONS_QUERYPLACEMENTEXCEPTION_HPP_
