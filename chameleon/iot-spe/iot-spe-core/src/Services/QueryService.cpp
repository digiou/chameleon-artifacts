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

#include <Catalogs/Source/SourceCatalog.hpp>
#include <Catalogs/UDF/UDFCatalog.hpp>
#include <Configurations/Coordinator/OptimizerConfiguration.hpp>
#include <Exceptions/InvalidArgumentException.hpp>
#include <Exceptions/InvalidQueryException.hpp>
#include <Optimizer/QueryPlacement/PlacementStrategyFactory.hpp>
#include <Optimizer/QueryValidation/SemanticQueryValidation.hpp>
#include <Optimizer/QueryValidation/SyntacticQueryValidation.hpp>
#include <Plans/Global/Execution/ExecutionNode.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Plans/Utils/PlanIdGenerator.hpp>
#include <Plans/Utils/QueryPlanIterator.hpp>
#include <RequestProcessor/AsyncRequestProcessor.hpp>
#include <RequestProcessor/RequestTypes/AddQueryRequest.hpp>
#include <RequestProcessor/RequestTypes/FailQueryRequest.hpp>
#include <RequestProcessor/RequestTypes/StopQueryRequest.hpp>
#include <RequestProcessor/StorageHandles/TwoPhaseLockingStorageHandler.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/PlacementStrategy.hpp>
#include <WorkQueues/RequestQueue.hpp>
#include <WorkQueues/RequestTypes/QueryRequests/AddQueryRequest.hpp>
#include <WorkQueues/RequestTypes/QueryRequests/FailQueryRequest.hpp>
#include <WorkQueues/RequestTypes/QueryRequests/StopQueryRequest.hpp>

namespace x {

QueryService::QueryService(bool enableNewRequestExecutor,
                           Configurations::OptimizerConfiguration optimizerConfiguration,
                           const QueryCatalogServicePtr& queryCatalogService,
                           const RequestQueuePtr& queryRequestQueue,
                           const Catalogs::Source::SourceCatalogPtr& sourceCatalog,
                           const QueryParsingServicePtr& queryParsingService,
                           const Catalogs::UDF::UDFCatalogPtr& udfCatalog,
                           const RequestProcessor::Experimental::AsyncRequestProcessorPtr& asyncRequestExecutor,
                           const z3::ContextPtr& z3Context)
    : enableNewRequestExecutor(enableNewRequestExecutor), optimizerConfiguration(optimizerConfiguration),
      queryCatalogService(queryCatalogService), queryRequestQueue(queryRequestQueue), asyncRequestExecutor(asyncRequestExecutor),
      z3Context(z3Context), queryParsingService(queryParsingService) {
    x_DEBUG("QueryService()");
    syntacticQueryValidation = Optimizer::SyntacticQueryValidation::create(this->queryParsingService);
    semanticQueryValidation = Optimizer::SemanticQueryValidation::create(sourceCatalog,
                                                                         optimizerConfiguration.performAdvanceSemanticValidation,
                                                                         udfCatalog);
}

QueryId QueryService::validateAndQueueAddQueryRequest(const std::string& queryString,
                                                      const Optimizer::PlacementStrategy placementStrategy,
                                                      const FaultToleranceType faultTolerance,
                                                      const LineageType lineage) {

    if (!enableNewRequestExecutor) {
        x_INFO("QueryService: Validating and registering the user query.");
        QueryId queryId = PlanIdGenerator::getNextQueryId();
        try {
            // Checking the syntactic validity and compiling the query string to an object
            x_INFO("QueryService: check validation of a query.");
            QueryPlanPtr queryPlan = syntacticQueryValidation->validate(queryString);

            queryPlan->setQueryId(queryId);
            queryPlan->setFaultToleranceType(faultTolerance);
            queryPlan->setLineageType(lineage);

            // perform semantic validation
            semanticQueryValidation->validate(queryPlan);

            Catalogs::Query::QueryCatalogEntryPtr queryCatalogEntry =
                queryCatalogService->createNewEntry(queryString, queryPlan, placementStrategy);
            if (queryCatalogEntry) {
                auto request = AddQueryRequest::create(queryPlan, placementStrategy);
                queryRequestQueue->add(request);
                return queryId;
            }
        } catch (const InvalidQueryException& exc) {
            x_ERROR("QueryService: {}", std::string(exc.what()));
            auto emptyQueryPlan = QueryPlan::create();
            emptyQueryPlan->setQueryId(queryId);
            queryCatalogService->createNewEntry(queryString, emptyQueryPlan, placementStrategy);
            queryCatalogService->updateQueryStatus(queryId, QueryState::FAILED, exc.what());
            throw exc;
        }
        throw Exceptions::RuntimeException("QueryService: unable to create query catalog entry");
    } else {

        auto addRequest = RequestProcessor::Experimental ::AddQueryRequest::create(queryString,
                                                                                   placementStrategy,
                                                                                   faultTolerance,
                                                                                   lineage,
                                                                                   1,
                                                                                   z3Context,
                                                                                   queryParsingService);
        asyncRequestExecutor->runAsync(addRequest);
        auto future = addRequest->getFuture();
        return std::static_pointer_cast<RequestProcessor::Experimental::AddQueryResponse>(future.get())->queryId;
    }
}

QueryId QueryService::addQueryRequest(const std::string& queryString,
                                      const QueryPlanPtr& queryPlan,
                                      const Optimizer::PlacementStrategy placementStrategy,
                                      const FaultToleranceType faultTolerance,
                                      const LineageType lineage) {

    if (!enableNewRequestExecutor) {
        QueryId queryId = PlanIdGenerator::getNextQueryId();
        auto promise = std::make_shared<std::promise<QueryId>>();
        try {

            //Assign additional configurations
            queryPlan->setQueryId(queryId);
            queryPlan->setFaultToleranceType(faultTolerance);
            queryPlan->setLineageType(lineage);

            // assign the id for the query and individual operators
            assignOperatorIds(queryPlan);

            // perform semantic validation
            semanticQueryValidation->validate(queryPlan);

            Catalogs::Query::QueryCatalogEntryPtr queryCatalogEntry =
                queryCatalogService->createNewEntry(queryString, queryPlan, placementStrategy);
            if (queryCatalogEntry) {
                auto request = AddQueryRequest::create(queryPlan, placementStrategy);
                queryRequestQueue->add(request);
                return queryId;
            }
        } catch (const InvalidQueryException& exc) {
            x_ERROR("QueryService: {}", std::string(exc.what()));
            auto emptyQueryPlan = QueryPlan::create();
            emptyQueryPlan->setQueryId(queryId);
            queryCatalogService->createNewEntry(queryString, emptyQueryPlan, placementStrategy);
            queryCatalogService->updateQueryStatus(queryId, QueryState::FAILED, exc.what());
            throw exc;
        }
        throw Exceptions::RuntimeException("QueryService: unable to create query catalog entry");
    } else {
        auto addRequest = RequestProcessor::Experimental::AddQueryRequest::create(queryPlan,
                                                                                  placementStrategy,
                                                                                  faultTolerance,
                                                                                  lineage,
                                                                                  1,
                                                                                  z3Context);
        asyncRequestExecutor->runAsync(addRequest);
        auto future = addRequest->getFuture();
        return std::static_pointer_cast<RequestProcessor::Experimental::AddQueryResponse>(future.get())->queryId;
    }
}

bool QueryService::validateAndQueueStopQueryRequest(QueryId queryId) {

    if (!enableNewRequestExecutor) {
        //Check and mark query for hard stop
        bool success = queryCatalogService->checkAndMarkForHardStop(queryId);

        //If success then queue the hard stop request
        if (success) {
            auto request = StopQueryRequest::create(queryId);
            return queryRequestQueue->add(request);
        }
        return false;
    } else {
        auto stopRequest = RequestProcessor::Experimental::StopQueryRequest::create(queryId, 1);
        asyncRequestExecutor->runAsync(stopRequest);
        return true;
    }
}

bool QueryService::validateAndQueueFailQueryRequest(SharedQueryId sharedQueryId,
                                                    QuerySubPlanId querySubPlanId,
                                                    const std::string& failureReason) {

    if (!enableNewRequestExecutor) {
        auto request = FailQueryRequest::create(sharedQueryId, failureReason);
        return queryRequestQueue->add(request);
    } else {
        auto stopRequest = RequestProcessor::Experimental::FailQueryRequest::create(sharedQueryId, querySubPlanId, 1);
        asyncRequestExecutor->runAsync(stopRequest);
        return true;
    }
}

void QueryService::assignOperatorIds(QueryPlanPtr queryPlan) {
    // Iterate over all operators in the query and replace the client-provided ID
    auto queryPlanIterator = QueryPlanIterator(queryPlan);
    for (auto itr = queryPlanIterator.begin(); itr != QueryPlanIterator::end(); ++itr) {
        auto visitingOp = (*itr)->as<OperatorNode>();
        visitingOp->setId(Util::getNextOperatorId());
    }
}

}// namespace x
