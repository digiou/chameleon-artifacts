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

#include <WorkQueues/RequestTypes/QueryRequests/StopQueryRequest.hpp>
#include <string>
namespace x {

StopQueryRequest::StopQueryRequest(QueryId queryId) : queryId(queryId) {}

StopQueryRequestPtr StopQueryRequest::create(QueryId queryId) {
    return std::make_shared<StopQueryRequest>(StopQueryRequest(queryId));
}

std::string StopQueryRequest::toString() { return "StopQueryRequest { QueryId: " + std::to_string(queryId) + "}"; }

QueryId StopQueryRequest::getQueryId() const { return queryId; }

RequestType StopQueryRequest::getRequestType() { return RequestType::StopQuery; }

}// namespace x