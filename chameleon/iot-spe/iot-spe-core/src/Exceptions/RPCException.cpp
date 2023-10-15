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
#include <Exceptions/RpcException.hpp>
#include <utility>
namespace x::Exceptions {
RpcException::RpcException(const std::string& message, std::vector<RpcFailureInformation> failedRpcs, RpcClientModes mode)
    : RequestExecutionException(message), failedRpcs(std::move(failedRpcs)), mode(mode) {}

const char* RpcException::what() const noexcept { return message.c_str(); }

std::vector<RpcFailureInformation> RpcException::getFailedCalls() { return failedRpcs; }

RpcClientModes RpcException::getMode() { return mode; }
}// namespace x::Exceptions