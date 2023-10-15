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
#include <Exceptions/ExecutionNodeNotFoundException.hpp>
namespace x::Exceptions {
ExecutionNodeNotFoundException::ExecutionNodeNotFoundException(const std::string& message, NodeId id)
    : RequestExecutionException(message), id(id) {}

ExecutionNodeNotFoundException::ExecutionNodeNotFoundException(const std::string& message)
    : RequestExecutionException(message), id(INVALID_TOPOLOGY_NODE_ID) {}

NodeId ExecutionNodeNotFoundException::getNodeId() const { return id; }
}// namespace x::Exceptions
