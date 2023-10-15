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

#include <Exceptions/InvalidQueryStateException.hpp>
#include <Exceptions/QueryNotFoundException.hpp>
#include <Exceptions/QueryUndeploymentException.hpp>
#include <Exceptions/RuntimeException.hpp>
#include <Optimizer/Phases/QueryUndeploymentPhase.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Plans/Global/Query/SharedQueryPlan.hpp>
#include <RequestProcessor/RequestTypes/FailQueryRequest.hpp>
#include <RequestProcessor/StorageHandles/ResourceType.hpp>
#include <RequestProcessor/StorageHandles/StorageHandler.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Util/RequestType.hpp>

namespace x::RequestProcessor::Experimental {

FailQueryRequest::FailQueryRequest(const x::QueryId queryId,
                                   const x::QuerySubPlanId failedSubPlanId,
                                   const uint8_t maxRetries)
    : AbstractRequest({ResourceType::GlobalQueryPlan,
                       ResourceType::QueryCatalogService,
                       ResourceType::Topology,
                       ResourceType::GlobalExecutionPlan},
                      maxRetries),
      queryId(queryId), querySubPlanId(failedSubPlanId) {}

FailQueryRequestPtr FailQueryRequest::create(x::QueryId queryId, x::QuerySubPlanId failedSubPlanId, uint8_t maxRetries) {
    return std::make_shared<FailQueryRequest>(queryId, failedSubPlanId, maxRetries);
}

void FailQueryRequest::preRollbackHandle(const RequestExecutionException&, const StorageHandlerPtr&) {}

std::vector<AbstractRequestPtr> FailQueryRequest::rollBack(RequestExecutionException&, const StorageHandlerPtr&) { return {}; }

void FailQueryRequest::postRollbackHandle(const RequestExecutionException&, const StorageHandlerPtr&) {

    //todo #3727: perform error handling
}

void FailQueryRequest::postExecution(const StorageHandlerPtr& storageHandler) { storageHandler->releaseResources(queryId); }

std::vector<AbstractRequestPtr> FailQueryRequest::executeRequestLogic(const StorageHandlerPtr& storageHandle) {
    globalQueryPlan = storageHandle->getGlobalQueryPlanHandle(requestId);
    globalExecutionPlan = storageHandle->getGlobalExecutionPlanHandle(requestId);
    queryCatalogService = storageHandle->getQueryCatalogServiceHandle(requestId);
    topology = storageHandle->getTopologyHandle(requestId);
    auto sharedQueryId = globalQueryPlan->getSharedQueryId(queryId);
    if (sharedQueryId == INVALID_SHARED_QUERY_ID) {
        throw Exceptions::QueryNotFoundException("Could not find a query with the id " + std::to_string(queryId)
                                                 + " in the global query plan");
    }

    //respond to the calling service which is the shared query id o the query being undeployed
    responsePromise.set_value(std::make_shared<FailQueryResponse>(sharedQueryId));

    queryCatalogService->checkAndMarkForFailure(sharedQueryId, querySubPlanId);

    globalQueryPlan->removeQuery(queryId, RequestType::FailQuery);

    //undeploy queries
    try {
        auto queryUndeploymentPhase = QueryUndeploymentPhase::create(topology, globalExecutionPlan);
        queryUndeploymentPhase->execute(sharedQueryId, SharedQueryPlanStatus::Failed);
    } catch (x::Exceptions::RuntimeException& e) {
        throw Exceptions::QueryUndeploymentException(sharedQueryId,
                                                     "Failed to undeploy query with id " + std::to_string(queryId));
    }

    //update global query plan
    for (auto& id : globalQueryPlan->getSharedQueryPlan(sharedQueryId)->getQueryIds()) {
        queryCatalogService->updateQueryStatus(id, QueryState::FAILED, "Failed");
    }

    //no follow up requests
    return {};
}
}// namespace x::RequestProcessor::Experimental
