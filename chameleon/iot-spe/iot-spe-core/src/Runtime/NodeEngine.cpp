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
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Compiler/CPPCompiler/CPPCompiler.hpp>
#include <Compiler/JITCompilerBuilder.hpp>
#include <Exceptions/ErrorListener.hpp>
#include <Network/NetworkManager.hpp>
#include <Network/NetworkSink.hpp>
#include <Network/PartitionManager.hpp>
#include <Operators/LogicalOperators/JoinLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/LambdaSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Phases/ConvertLogicalToPhysicalSink.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/DefaultQueryCompiler.hpp>
#include <QueryCompiler/QueryCompilationRequest.hpp>
#include <QueryCompiler/QueryCompilationResult.hpp>
#include <QueryCompiler/QueryCompilerOptions.hpp>
#include <Runtime/Execution/ExecutablePipeline.hpp>
#include <Runtime/Execution/ExecutableQueryPlan.hpp>
#include <Runtime/MaterializedViewManager.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/QueryManager.hpp>
#include <Util/Logger/Logger.hpp>
#include <string>
#include <utility>

namespace x::Runtime {

NodeEngine::NodeEngine(std::vector<PhysicalSourcePtr> physicalSources,
                       HardwareManagerPtr&& hardwareManager,
                       std::vector<BufferManagerPtr>&& bufferManagers,
                       QueryManagerPtr&& queryManager,
                       std::function<Network::NetworkManagerPtr(std::shared_ptr<NodeEngine>)>&& networkManagerCreator,
                       Network::PartitionManagerPtr&& partitionManager,
                       QueryCompilation::QueryCompilerPtr&& queryCompiler,
                       StateManagerPtr&& stateManager,
                       std::weak_ptr<AbstractQueryStatusListener>&& xWorker,
                       x::Experimental::MaterializedView::MaterializedViewManagerPtr&& materializedViewManager,
                       OpenCLManagerPtr&& openCLManager,
                       uint64_t nodeEngineId,
                       uint64_t numberOfBuffersInGlobalBufferManager,
                       uint64_t numberOfBuffersInSourceLocalBufferPool,
                       uint64_t numberOfBuffersPerWorker,
                       bool sourceSharing)
    : nodeId(INVALID_TOPOLOGY_NODE_ID), physicalSources(std::move(physicalSources)), hardwareManager(std::move(hardwareManager)),
      bufferManagers(std::move(bufferManagers)), queryManager(std::move(queryManager)), queryCompiler(std::move(queryCompiler)),
      partitionManager(std::move(partitionManager)), stateManager(std::move(stateManager)), xWorker(std::move(xWorker)),
      materializedViewManager(std::move(materializedViewManager)), openCLManager(std::move(openCLManager)),
      nodeEngineId(nodeEngineId), numberOfBuffersInGlobalBufferManager(numberOfBuffersInGlobalBufferManager),
      numberOfBuffersInSourceLocalBufferPool(numberOfBuffersInSourceLocalBufferPool),
      numberOfBuffersPerWorker(numberOfBuffersPerWorker), sourceSharing(sourceSharing) {

    x_TRACE("Runtime() id={}", nodeEngineId);
    // here shared_from_this() does not work because of the machinery behind make_shared
    // as a result, we need to use a trick, i.e., a shared ptr that does not deallocate the node engine
    // plz make sure that ExchangeProtocol never leaks the impl pointer
    // TODO refactor to decouple the two components!
    networkManager = networkManagerCreator(std::shared_ptr<NodeEngine>(this, [](NodeEngine*) {
        // nop
    }));
    if (!this->queryManager->startThreadPool(numberOfBuffersPerWorker)) {
        x_ERROR("Runtime: error while start thread pool");
        throw Exceptions::RuntimeException("Error while starting thread pool");
    }
    x_DEBUG("NodeEngine(): thread pool successfully started");

    isRunning.store(true);
}

NodeEngine::~NodeEngine() {
    x_DEBUG("Destroying Runtime()");
    x_ASSERT(stop(), "Cannot stop node engine");
}

bool NodeEngine::deployQueryInNodeEngine(const Execution::ExecutableQueryPlanPtr& queryExecutionPlan) {
    std::unique_lock lock(engineMutex);
    x_DEBUG("Runtime: deployQueryInNodeEngine query using qep with queryId: {}", queryExecutionPlan->getQueryId());
    bool successRegister = registerQueryInNodeEngine(queryExecutionPlan);
    if (!successRegister) {
        x_ERROR("Runtime::deployQueryInNodeEngine: failed to register query");
        return false;
    }
    x_DEBUG("Runtime::deployQueryInNodeEngine: successfully register query");

    bool successStart = startQuery(queryExecutionPlan->getQueryId());
    if (!successStart) {
        x_ERROR("Runtime::deployQueryInNodeEngine: failed to start query");
        return false;
    }
    x_DEBUG("Runtime::deployQueryInNodeEngine: successfully start query");

    return true;
}

bool NodeEngine::registerQueryInNodeEngine(const QueryPlanPtr& queryPlan) {
    QueryId queryId = queryPlan->getQueryId();
    QueryId querySubPlanId = queryPlan->getQuerySubPlanId();

    x_INFO("Creating ExecutableQueryPlan for {} {} ", queryId, querySubPlanId);

    auto request = QueryCompilation::QueryCompilationRequest::create(queryPlan, inherited1::shared_from_this());
    request->enableDump();
    auto result = queryCompiler->compileQuery(request);
    try {
        auto executablePlan = result->getExecutableQueryPlan();
        return registerQueryInNodeEngine(executablePlan);
    } catch (std::exception const& error) {
        x_ERROR("Error while building query execution plan: {}", error.what());
        x_ASSERT(false, "Error while building query execution plan: " << error.what());
        return false;
    }
}

bool NodeEngine::registerQueryInNodeEngine(const Execution::ExecutableQueryPlanPtr& queryExecutionPlan) {
    std::unique_lock lock(engineMutex);
    QueryId queryId = queryExecutionPlan->getQueryId();
    QuerySubPlanId querySubPlanId = queryExecutionPlan->getQuerySubPlanId();
    x_DEBUG("Runtime: registerQueryInNodeEngine query with queryId= {} querySubPlanId = {}", queryId, querySubPlanId);
    x_ASSERT(queryManager->isThreadPoolRunning(), "Registering query but thread pool not running");
    if (deployedQEPs.find(querySubPlanId) == deployedQEPs.end()) {
        auto found = queryIdToQuerySubPlanIds.find(queryId);
        if (found == queryIdToQuerySubPlanIds.end()) {
            queryIdToQuerySubPlanIds[queryId] = {querySubPlanId};
            x_DEBUG("Runtime: register of QEP  {}  as a singleton", querySubPlanId);
        } else {
            (*found).second.push_back(querySubPlanId);
            x_DEBUG("Runtime: register of QEP  {}  added", querySubPlanId);
        }
        if (queryManager->registerQuery(queryExecutionPlan)) {
            deployedQEPs[querySubPlanId] = queryExecutionPlan;
            x_DEBUG("Runtime: register of subqep  {}  succeeded", querySubPlanId);
            return true;
        }
        x_DEBUG("Runtime: register of subqep  {}  failed", querySubPlanId);
        return false;

    } else {
        x_DEBUG("Runtime: qep already exists. register failed {}", querySubPlanId);
        return false;
    }
}

bool NodeEngine::startQuery(QueryId queryId) {
    std::unique_lock lock(engineMutex);
    x_DEBUG("Runtime: startQuery= {}", queryId);
    if (queryIdToQuerySubPlanIds.find(queryId) != queryIdToQuerySubPlanIds.end()) {

        std::vector<QuerySubPlanId> querySubPlanIds = queryIdToQuerySubPlanIds[queryId];
        if (querySubPlanIds.empty()) {
            x_ERROR("Runtime: Unable to find qep ids for the query {}. Start failed.", queryId);
            return false;
        }

        for (auto querySubPlanId : querySubPlanIds) {
            try {
                if (queryManager->startQuery(deployedQEPs[querySubPlanId])) {
                    x_DEBUG("Runtime: start of QEP  {}  succeeded", querySubPlanId);
                } else {
                    x_DEBUG("Runtime: start of QEP  {}  failed", querySubPlanId);
                    return false;
                }
            } catch (std::exception const& exception) {
                x_ERROR("Got exception while starting query {}", exception.what());
            }
        }
        return true;
    }
    x_ERROR("Runtime: qep does not exists. start failed for query={}", queryId);
    return false;
}

bool NodeEngine::undeployQuery(QueryId queryId) {
    std::unique_lock lock(engineMutex);
    x_DEBUG("Runtime: undeployQuery query= {}", queryId);
    bool successStop = stopQuery(queryId);
    if (!successStop) {
        x_ERROR("Runtime::undeployQuery: failed to stop query");
        return false;
    }
    x_DEBUG("Runtime::undeployQuery: successfully stop query");

    bool successUnregister = unregisterQuery(queryId);
    if (!successUnregister) {
        x_ERROR("Runtime::undeployQuery: failed to unregister query");
        return false;
    }
    x_DEBUG("Runtime::undeployQuery: successfully unregister query");
    return true;
}

bool NodeEngine::unregisterQuery(QueryId queryId) {
    std::unique_lock lock(engineMutex);
    x_DEBUG("Runtime: unregisterQuery query= {}", queryId);
    bool ret = true;
    if (auto it = queryIdToQuerySubPlanIds.find(queryId); it != queryIdToQuerySubPlanIds.end()) {
        auto& querySubPlanIds = it->second;
        if (querySubPlanIds.empty()) {
            x_ERROR("Runtime: Unable to find qep ids for the query {}. Start failed.", queryId);
            return false;
        }

        for (const auto querySubPlanId : querySubPlanIds) {
            auto qep = deployedQEPs[querySubPlanId];
            bool isStopped = false;
            switch (qep->getStatus()) {
                case Execution::ExecutableQueryPlanStatus::Created:
                case Execution::ExecutableQueryPlanStatus::Deployed:
                case Execution::ExecutableQueryPlanStatus::Running: {
                    x_DEBUG("Runtime: unregister of query  {}  is not Stopped... stopping now", querySubPlanId);
                    isStopped = queryManager->stopQuery(qep, Runtime::QueryTerminationType::HardStop);
                    break;
                }
                default: {
                    isStopped = true;
                    break;
                };
            }
            x_DEBUG("Runtime: unregister of query  {} : current status is stopped= {}", querySubPlanId, isStopped);
            if (isStopped && queryManager->deregisterQuery(qep)) {
                deployedQEPs.erase(querySubPlanId);
                x_DEBUG("Runtime: unregister of query  {}  succeeded", querySubPlanId);
            } else {
                x_ERROR("Runtime: unregister of QEP {} failed", querySubPlanId);
                ret = false;
            }
        }
        queryIdToQuerySubPlanIds.erase(queryId);
        return true;
    }

    x_ERROR("Runtime: qep does not exists. unregister failed{}", queryId);
    return false;
}

bool NodeEngine::stopQuery(QueryId queryId, Runtime::QueryTerminationType terminationType) {
    std::unique_lock lock(engineMutex);
    x_DEBUG("Runtime:stopQuery for qep= {}  termination= {}", queryId, terminationType);
    auto it = queryIdToQuerySubPlanIds.find(queryId);
    if (it != queryIdToQuerySubPlanIds.end()) {
        std::vector<QuerySubPlanId> querySubPlanIds = it->second;
        if (querySubPlanIds.empty()) {
            x_ERROR("Runtime: Unable to find qep ids for the query {}. Stop failed.", queryId);
            return false;
        }

        switch (terminationType) {
            case QueryTerminationType::Graceful:
            case QueryTerminationType::HardStop: {
                for (auto querySubPlanId : querySubPlanIds) {
                    try {
                        if (queryManager->stopQuery(deployedQEPs[querySubPlanId], terminationType)) {
                            x_DEBUG("Runtime: stop of QEP  {}  succeeded", querySubPlanId);
                            return true;
                        } else {
                            x_ERROR("Runtime: stop of QEP {} failed", querySubPlanId);
                            return false;
                        }
                    } catch (std::exception const& exception) {
                        x_ERROR("Got exception while stopping query {}", exception.what());
                        return false;// handle this better!
                    }
                }
            }
            case QueryTerminationType::Failure: {
                for (auto querySubPlanId : querySubPlanIds) {
                    try {
                        if (queryManager->failQuery(deployedQEPs[querySubPlanId])) {
                            x_DEBUG("Runtime: failure of QEP  {}  succeeded", querySubPlanId);
                            return true;
                        } else {
                            x_ERROR("Runtime: failure of QEP {} failed", querySubPlanId);
                            return false;
                        }
                    } catch (std::exception const& exception) {
                        x_ERROR("Got exception while stopping query {}", exception.what());
                        return false;// handle this better!
                    }
                }
            }
            case QueryTerminationType::Invalid: x_NOT_IMPLEMENTED();
        }
        return true;
    }
    x_ERROR("Runtime: qep does not exists. stop failed {}", queryId);
    return false;
}

QueryManagerPtr NodeEngine::getQueryManager() { return queryManager; }

bool NodeEngine::stop(bool markQueriesAsFailed) {
    //TODO: add check if still queryIdAndCatalogEntryMapping are running
    //TODO @Steffen: does it make sense to have force stop still?
    //TODO @all: imho, when this method terminates, nothing must be running still and all resources must be returned to the engine
    //TODO @all: error handling, e.g., is it an error if the query is stopped but not undeployed? @Steffen?

    bool expected = true;
    if (!isRunning.compare_exchange_strong(expected, false)) {
        x_WARNING("Runtime::stop: engine already stopped");
        return true;
    }
    x_DEBUG("Runtime::stop: going to stop the node engine");
    std::unique_lock lock(engineMutex);
    bool withError = false;

    // release all deployed queryIdAndCatalogEntryMapping
    for (auto it = deployedQEPs.begin(); it != deployedQEPs.end();) {
        auto& [querySubPlanId, queryExecutionPlan] = *it;
        try {
            if (markQueriesAsFailed) {
                if (queryManager->failQuery(queryExecutionPlan)) {
                    x_DEBUG("Runtime: fail of QEP  {}  succeeded", querySubPlanId);
                } else {
                    x_ERROR("Runtime: fail of QEP {} failed", querySubPlanId);
                    withError = true;
                }
            } else {
                if (queryManager->stopQuery(queryExecutionPlan)) {
                    x_DEBUG("Runtime: stop of QEP  {}  succeeded", querySubPlanId);
                } else {
                    x_ERROR("Runtime: stop of QEP {} failed", querySubPlanId);
                    withError = true;
                }
            }
        } catch (std::exception const& err) {
            x_ERROR("Runtime: stop of QEP {} failed: {}", querySubPlanId, err.what());
            withError = true;
        }
        try {
            if (queryManager->deregisterQuery(queryExecutionPlan)) {
                x_DEBUG("Runtime: deregisterQuery of QEP  {}  succeeded", querySubPlanId);
                it = deployedQEPs.erase(it);
            } else {
                x_ERROR("Runtime: deregisterQuery of QEP {} failed", querySubPlanId);
                withError = true;
                ++it;
            }
        } catch (std::exception const& err) {
            x_ERROR("Runtime: deregisterQuery of QEP {} failed: {}", querySubPlanId, err.what());
            withError = true;
            ++it;
        }
    }
    // release components
    // TODO do not touch the sequence here as it will lead to errors in the shutdown sequence
    deployedQEPs.clear();
    queryIdToQuerySubPlanIds.clear();
    queryManager->destroy();
    networkManager->destroy();
    partitionManager->clear();
    for (auto&& bufferManager : bufferManagers) {
        bufferManager->destroy();
    }
    stateManager->destroy();
    xWorker.reset();// break cycle
    return !withError;
}

BufferManagerPtr NodeEngine::getBufferManager(uint32_t bufferManagerIndex) const {
    x_ASSERT2_FMT(bufferManagerIndex < bufferManagers.size(), "invalid buffer manager index=" << bufferManagerIndex);
    return bufferManagers[bufferManagerIndex];
}

void NodeEngine::injectEpochBarrier(uint64_t timestamp, uint64_t queryId) const {
    std::unique_lock lock(engineMutex);
    std::vector<QuerySubPlanId> subQueryPlanIds = queryIdToQuerySubPlanIds.find(queryId)->second;
    for (auto& subQueryPlanId : subQueryPlanIds) {
        x_DEBUG("NodeEngine: Find sources for subQueryPlanId  {}", subQueryPlanId);
        auto sources = deployedQEPs.find(subQueryPlanId)->second->getSources();
        for (auto& source : sources) {
            if (source->injectEpochBarrier(timestamp, queryId)) {
                x_DEBUG("NodeEngine: Inject epoch barrier  {} to the query  {}", timestamp, queryId);
            } else {
                x_ERROR("NodeEngine: Couldn't inject epoch barrier to the query {}", queryId);
            }
        }
    }
}

StateManagerPtr NodeEngine::getStateManager() { return stateManager; }

uint64_t NodeEngine::getNodeEngineId() { return nodeEngineId; }

Network::NetworkManagerPtr NodeEngine::getNetworkManager() { return networkManager; }

AbstractQueryStatusListenerPtr NodeEngine::getQueryStatusListener() { return xWorker; }

HardwareManagerPtr NodeEngine::getHardwareManager() const { return hardwareManager; }

x::Experimental::MaterializedView::MaterializedViewManagerPtr NodeEngine::getMaterializedViewManager() const {
    return materializedViewManager;
}

Execution::ExecutableQueryPlanStatus NodeEngine::getQueryStatus(QueryId queryId) {
    std::unique_lock lock(engineMutex);
    if (queryIdToQuerySubPlanIds.find(queryId) != queryIdToQuerySubPlanIds.end()) {
        std::vector<QuerySubPlanId> querySubPlanIds = queryIdToQuerySubPlanIds[queryId];
        if (querySubPlanIds.empty()) {
            x_ERROR("Runtime: Unable to find qep ids for the query {}. Start failed.", queryId);
            return Execution::ExecutableQueryPlanStatus::Invalid;
        }

        for (auto querySubPlanId : querySubPlanIds) {
            //FIXME: handle vector of statistics properly in #977
            return deployedQEPs[querySubPlanId]->getStatus();
        }
    }
    return Execution::ExecutableQueryPlanStatus::Invalid;
}

void NodeEngine::onDataBuffer(Network::xPartition, TupleBuffer&) {
    // nop :: kept as legacy
}

void NodeEngine::onEvent(x::Network::xPartition, x::Runtime::BaseEvent&) {
    // nop :: kept as legacy
}

void NodeEngine::onEndOfStream(Network::Messages::EndOfStreamMessage) {
    // nop :: kept as legacy
}

void NodeEngine::onServerError(Network::Messages::ErrorMessage err) {

    switch (err.getErrorType()) {
        case Network::Messages::ErrorType::PartitionNotRegisteredError: {
            x_WARNING("Runtime: Unable to find the x Partition {}", err.getChannelId());
            break;
        }
        case Network::Messages::ErrorType::DeletedPartitionError: {
            x_WARNING("Runtime: Requesting deleted x Partition {}", err.getChannelId());
            break;
        }
        default: {
            x_ASSERT(false, err.getErrorTypeAsString());
            break;
        }
    }
}

void NodeEngine::onChannelError(Network::Messages::ErrorMessage err) {
    switch (err.getErrorType()) {
        case Network::Messages::ErrorType::PartitionNotRegisteredError: {
            x_WARNING("Runtime: Unable to find the x Partition {}", err.getChannelId());
            break;
        }
        case Network::Messages::ErrorType::DeletedPartitionError: {
            x_WARNING("Runtime: Requesting deleted x Partition {}", err.getChannelId());
            break;
        }
        default: {
            x_THROW_RUNTIME_ERROR(err.getErrorTypeAsString());
            break;
        }
    }
}

std::vector<QueryStatisticsPtr> NodeEngine::getQueryStatistics(QueryId queryId) {
    x_INFO("QueryManager: Get query statistics for query {}", queryId);
    std::unique_lock lock(engineMutex);
    std::vector<QueryStatisticsPtr> queryStatistics;

    x_TRACE("QueryManager: Check if query is registered");
    auto foundQuerySubPlanIds = queryIdToQuerySubPlanIds.find(queryId);
    x_TRACE("Found members = {}", foundQuerySubPlanIds->second.size());
    if (foundQuerySubPlanIds == queryIdToQuerySubPlanIds.end()) {
        x_ERROR("AbstractQueryManager::getQueryStatistics: query does not exists {}", queryId);
        return queryStatistics;
    }

    x_TRACE("QueryManager: Extracting query execution ids for the input query {}", queryId);
    std::vector<QuerySubPlanId> querySubPlanIds = (*foundQuerySubPlanIds).second;
    for (auto querySubPlanId : querySubPlanIds) {
        queryStatistics.emplace_back(queryManager->getQueryStatistics(querySubPlanId));
    }
    return queryStatistics;
}

std::vector<QueryStatistics> NodeEngine::getQueryStatistics(bool withReset) {
    std::unique_lock lock(engineMutex);
    std::vector<QueryStatistics> queryStatistics;

    for (auto& plan : queryIdToQuerySubPlanIds) {
        x_TRACE("QueryManager: Extracting query execution ids for the input query {}", plan.first);
        std::vector<QuerySubPlanId> querySubPlanIds = plan.second;
        for (auto querySubPlanId : querySubPlanIds) {
            x_TRACE("querySubPlanId={} stat= {}",
                      querySubPlanId,
                      queryManager->getQueryStatistics(querySubPlanId)->getQueryStatisticsAsString());

            queryStatistics.push_back(queryManager->getQueryStatistics(querySubPlanId).operator*());
            if (withReset) {
                queryManager->getQueryStatistics(querySubPlanId)->clear();
            }
        }
    }

    return queryStatistics;
}

Network::PartitionManagerPtr NodeEngine::getPartitionManager() { return partitionManager; }

std::vector<QuerySubPlanId> NodeEngine::getSubQueryIds(uint64_t queryId) {
    auto iterator = queryIdToQuerySubPlanIds.find(queryId);
    if (iterator != queryIdToQuerySubPlanIds.end()) {
        return iterator->second;
    } else {
        return {};
    }
}

void NodeEngine::onFatalError(int signalNumber, std::string callstack) {
    x_ERROR("onFatalError: signal [{}] error [{}] callstack {}", signalNumber, strerror(errno), callstack);
    std::cerr << "Runtime failed fatally" << std::endl;// it's necessary for testing and it wont harm us to write to stderr
    std::cerr << "Error: " << strerror(errno) << std::endl;
    std::cerr << "Signal: " << std::to_string(signalNumber) << std::endl;
    std::cerr << "Callstack:\n " << callstack << std::endl;
#ifdef ENABLE_CORE_DUMPER
    detail::createCoreDump();
#endif
}

void NodeEngine::onFatalException(const std::shared_ptr<std::exception> exception, std::string callstack) {
    x_ERROR("onFatalException: exception=[{}] callstack={}", exception->what(), callstack);
    std::cerr << "Runtime failed fatally" << std::endl;
    std::cerr << "Error: " << strerror(errno) << std::endl;
    std::cerr << "Exception: " << exception->what() << std::endl;
    std::cerr << "Callstack:\n " << callstack << std::endl;
#ifdef ENABLE_CORE_DUMPER
    detail::createCoreDump();
#endif
}

const std::vector<PhysicalSourcePtr>& NodeEngine::getPhysicalSources() const { return physicalSources; }

std::shared_ptr<const Execution::ExecutableQueryPlan> NodeEngine::getExecutableQueryPlan(uint64_t querySubPlanId) const {
    std::unique_lock lock(engineMutex);
    auto iterator = deployedQEPs.find(querySubPlanId);
    if (iterator != deployedQEPs.end()) {
        return iterator->second;
    }
    return nullptr;
}

bool NodeEngine::bufferData(QuerySubPlanId querySubPlanId, uint64_t uniqueNetworkSinkDescriptorId) {
    //TODO: #2412 add error handling/return false in some cases
    x_DEBUG("NodeEngine: Received request to buffer Data on network Sink");
    std::unique_lock lock(engineMutex);
    if (deployedQEPs.find(querySubPlanId) == deployedQEPs.end()) {
        x_DEBUG("Deployed QEP with ID:  {}  not found", querySubPlanId);
        return false;
    } else {
        auto qep = deployedQEPs.at(querySubPlanId);
        auto sinks = qep->getSinks();
        //make sure that query sub plan has network sink with specified id
        auto it = std::find_if(sinks.begin(), sinks.end(), [uniqueNetworkSinkDescriptorId](const DataSinkPtr& dataSink) {
            Network::NetworkSinkPtr networkSink = std::dynamic_pointer_cast<Network::NetworkSink>(dataSink);
            return networkSink && networkSink->getUniqueNetworkSinkDescriptorId() == uniqueNetworkSinkDescriptorId;
        });
        if (it != sinks.end()) {
            auto networkSink = *it;
            //below code will be added in #2395
            //ReconfigurationMessage message = ReconfigurationMessage(querySubPlanId,BufferData,networkSink);
            //queryManager->addReconfigurationMessage(querySubPlanId,message,true);
            x_NOT_IMPLEMENTED();
            return true;
        }
        //query sub plan did not have network sink with specified id
        x_DEBUG("Query Sub Plan with ID {} did not contain a Network Sink with a Descriptor with ID {}",
                  querySubPlanId,
                  uniqueNetworkSinkDescriptorId);
        return false;
    }
}

bool NodeEngine::bufferAllData() {
    x_DEBUG("NodeEngine of Node  {}  received request to buffer all outgoing data", nodeId);
    std::unique_lock lock(engineMutex);
    //iterate over all sinks at this node by first iterating over qeps and then for every qep iterate over its sinks
    for (auto& [qepId, qepPtr] : deployedQEPs) {
        auto sinks = qepPtr->getSinks();
        for (auto& sink : sinks) {
            //check if sink is a network sink
            auto networkSink = std::dynamic_pointer_cast<Network::NetworkSink>(sink);
            //whenever we encounter a network sink, send a reconfig message telling it to start buffering
            if (networkSink) {
                x_DEBUG("Starting to buffer on Network Sink{}", networkSink->getUniqueNetworkSinkDescriptorId());
                ReconfigurationMessage message = ReconfigurationMessage(qepPtr->getQueryId(),
                                                                        qepId,
                                                                        Runtime::ReconfigurationType::StartBuffering,
                                                                        networkSink);
                queryManager->addReconfigurationMessage(qepPtr->getQueryId(), qepId, message, true);
            } else {
                //if the sink is not a network sink, do nothing
                x_DEBUG("Sink is not a network sink");
            }
        }
    }
    return true;
}

bool NodeEngine::stopBufferingAllData() {
    x_DEBUG("NodeEngine of Node  {}  received request to stop buffering data", nodeId);
    std::unique_lock lock(engineMutex);
    //iterate over all sinks at this node by first iterating over qeps and then for every qep iterate over its sinks
    for (auto& [qepId, qepPtr] : deployedQEPs) {
        auto sinks = qepPtr->getSinks();
        for (auto& sink : sinks) {
            //check if sink is a network sink
            auto networkSink = std::dynamic_pointer_cast<Network::NetworkSink>(sink);
            //whenever we encounter a network sink, send a reconfig message telling it to stop buffering
            if (networkSink) {
                ReconfigurationMessage message =
                    ReconfigurationMessage(qepPtr->getQueryId(), qepId, Runtime::ReconfigurationType::StopBuffering, networkSink);
                queryManager->addReconfigurationMessage(qepPtr->getQueryId(), qepId, message, true);
            }
        }
    }
    return true;
}

bool NodeEngine::updateNetworkSink(uint64_t newNodeId,
                                   const std::string& newHostname,
                                   uint32_t newPort,
                                   QuerySubPlanId querySubPlanId,
                                   uint64_t uniqueNetworkSinkDescriptorId) {
    //TODO: #2412 add error handling/return false in some cases
    x_ERROR("NodeEngine: Received request to update Network Sink");
    Network::NodeLocation newNodeLocation(newNodeId, newHostname, newPort);
    std::unique_lock lock(engineMutex);
    if (deployedQEPs.find(querySubPlanId) == deployedQEPs.end()) {
        x_DEBUG("Deployed QEP with ID:  {}  not found", querySubPlanId);
        return false;
    } else {
        auto qep = deployedQEPs.at(querySubPlanId);
        auto networkSinks = qep->getSinks();
        //make sure that query sub plan has network sink with specified id
        auto it =
            std::find_if(networkSinks.begin(), networkSinks.end(), [uniqueNetworkSinkDescriptorId](const DataSinkPtr& dataSink) {
                Network::NetworkSinkPtr networkSink = std::dynamic_pointer_cast<Network::NetworkSink>(dataSink);
                return networkSink && networkSink->getUniqueNetworkSinkDescriptorId() == uniqueNetworkSinkDescriptorId;
            });
        if (it != networkSinks.end()) {
            auto networkSink = *it;
            //below code will be added in #2402
            //ReconfigurationMessage message = ReconfigurationMessage(querySubPlanId,UpdateSinks,networkSink, newNodeLocation);
            //queryManager->addReconfigurationMessage(querySubPlanId,message,true);
            x_NOT_IMPLEMENTED();
            return true;
        }
        //query sub plan did not have network sink with specified id
        x_DEBUG("Query Sub Plan with ID {} did not contain a Network Sink with a Descriptor with ID {}",
                  querySubPlanId,
                  uniqueNetworkSinkDescriptorId);
        return false;
    }
}

Monitoring::MetricStorePtr NodeEngine::getMetricStore() { return metricStore; }
void NodeEngine::setMetricStore(Monitoring::MetricStorePtr metricStore) {
    x_ASSERT(metricStore != nullptr, "NodeEngine: MetricStore is null.");
    this->metricStore = metricStore;
}
TopologyNodeId NodeEngine::getNodeId() const { return nodeId; }
void NodeEngine::setNodeId(const TopologyNodeId NodeId) { nodeId = NodeId; }

void NodeEngine::updatePhysicalSources(const std::vector<PhysicalSourcePtr>& physicalSources) {
    this->physicalSources = std::move(physicalSources);
}

}// namespace x::Runtime
