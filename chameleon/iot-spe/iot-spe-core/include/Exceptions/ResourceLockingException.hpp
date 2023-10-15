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
#ifndef x_CORE_INCLUDE_EXCEPTIONS_RESOURCELOCKINGEXCEPTION_HPP_
#define x_CORE_INCLUDE_EXCEPTIONS_RESOURCELOCKINGEXCEPTION_HPP_
#include <Exceptions/RequestExecutionException.hpp>

/**
 * @brief This exception is raised, when a request was not able to lock a resource
 */
namespace x {
namespace RequestProcessor::Experimental {
enum class ResourceType : uint8_t;
}
namespace Exceptions {

class ResourceLockingException : public RequestExecutionException {
  public:
    ResourceLockingException(const std::string& message, RequestProcessor::Experimental::ResourceType resourceType);

    /**
     * @brief Access the type of the resource which could not be accessed
     * @return the resource type
     */
    [[nodiscard]] RequestProcessor::Experimental::ResourceType getResourceType() const;

  private:
    RequestProcessor::Experimental::ResourceType resourceType;
};
}// namespace Exceptions
}// namespace x
#endif// x_CORE_INCLUDE_EXCEPTIONS_RESOURCELOCKINGEXCEPTION_HPP_
