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

#include <Catalogs/Query/QueryCatalog.hpp>
#include <Catalogs/Source/LogicalSource.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <Catalogs/UDF/UDFCatalog.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Exceptions/ErrorListener.hpp>
#include <GRPC/WorkerRPCClient.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Plans/Global/Execution/ExecutionNode.hpp>
#include <Plans/Global/Execution/GlobalExecutionPlan.hpp>
#include <Plans/Global/Query/GlobalQueryPlan.hpp>
#include <Plans/Global/Query/SharedQueryPlan.hpp>
#include <REST/RestServer.hpp>
#include <RequestProcessor/AsyncRequestProcessor.hpp>
#include <RequestProcessor/StorageHandles/StorageDataStructures.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Services/LocationService.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Services/RequestProcessorService.hpp>
#include <Services/TopologyManagerService.hpp>
#include <Spatial/Index/LocationIndex.hpp>
#include <Util/Logger/Logger.hpp>
#include <WorkQueues/RequestQueue.hpp>
#include <grpcpp/server_builder.h>
#include <memory>
#include <thread>
#include <z3++.h>

//GRPC Includes
#include <Compiler/CPPCompiler/CPPCompiler.hpp>
#include <Compiler/JITCompilerBuilder.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <GRPC/CoordinatorRPCServer.hpp>
#include <Monitoring/MonitoringManager.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Services/MonitoringService.hpp>
#include <Services/QueryParsingService.hpp>
#include <Services/SourceCatalogService.hpp>

#include <GRPC/HealthCheckRPCServer.hpp>
#include <Health.pb.h>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Services/CoordinatorHealthCheckService.hpp>
#include <Topology/Topology.hpp>
#include <Util/ThreadNaming.hpp>
#include <grpcpp/ext/health_check_service_server_builder_option.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace x {

using namespace Configurations;

extern void Exceptions::installGlobalErrorListener(std::shared_ptr<ErrorListener> const&);

xCoordinator::xCoordinator(CoordinatorConfigurationPtr coordinatorConfiguration)
    : coordinatorConfiguration(std::move(coordinatorConfiguration)), restIp(this->coordinatorConfiguration->restIp),
      restPort(this->coordinatorConfiguration->restPort), rpcIp(this->coordinatorConfiguration->coordinatorIp),
      rpcPort(this->coordinatorConfiguration->rpcPort), enableMonitoring(this->coordinatorConfiguration->enableMonitoring) {
    x_DEBUG("xCoordinator() restIp={} restPort={} rpcIp={} rpcPort={}", restIp, restPort, rpcIp, rpcPort);
    setThreadName("xCoordinator");
    topology = Topology::create();

    // TODO make compiler backend configurable
    auto cppCompiler = Compiler::CPPCompiler::create();
    auto jitCompiler = Compiler::JITCompilerBuilder().registerLanguageCompiler(cppCompiler).build();
    auto queryParsingService = QueryParsingService::create(jitCompiler);

    auto locationIndex = std::make_shared<x::Spatial::Index::Experimental::LocationIndex>();

    sourceCatalog = std::make_shared<Catalogs::Source::SourceCatalog>(queryParsingService);
    globalExecutionPlan = GlobalExecutionPlan::create();
    queryCatalog = std::make_shared<Catalogs::Query::QueryCatalog>();

    sourceCatalogService = std::make_shared<SourceCatalogService>(sourceCatalog);
    topologyManagerService = std::make_shared<TopologyManagerService>(topology, locationIndex);
    queryRequestQueue = std::make_shared<RequestQueue>(this->coordinatorConfiguration->optimizer.queryBatchSize);
    globalQueryPlan = GlobalQueryPlan::create();

    queryCatalogService = std::make_shared<QueryCatalogService>(queryCatalog);

    z3::config cfg;
    cfg.set("timeout", 1000);
    cfg.set("model", false);
    cfg.set("type_check", false);
    auto z3Context = std::make_shared<z3::context>(cfg);

    queryRequestProcessorService = std::make_shared<RequestProcessorService>(globalExecutionPlan,
                                                                             topology,
                                                                             queryCatalogService,
                                                                             globalQueryPlan,
                                                                             sourceCatalog,
                                                                             udfCatalog,
                                                                             queryRequestQueue,
                                                                             this->coordinatorConfiguration,
                                                                             z3Context);

    RequestProcessor::Experimental::StorageDataStructures storageDataStructures = {this->coordinatorConfiguration,
                                                                                   topology,
                                                                                   globalExecutionPlan,
                                                                                   queryCatalogService,
                                                                                   globalQueryPlan,
                                                                                   sourceCatalog,
                                                                                   udfCatalog};

    auto asyncRequestExecutor = std::make_shared<RequestProcessor::Experimental::AsyncRequestProcessor>(storageDataStructures);
    bool enableNewRequestExecutor = this->coordinatorConfiguration->enableNewRequestExecutor.getValue();
    queryService = std::make_shared<QueryService>(enableNewRequestExecutor,
                                                  this->coordinatorConfiguration->optimizer,
                                                  queryCatalogService,
                                                  queryRequestQueue,
                                                  sourceCatalog,
                                                  queryParsingService,
                                                  udfCatalog,
                                                  asyncRequestExecutor,
                                                  z3Context);

    udfCatalog = Catalogs::UDF::UDFCatalog::create();
    locationService = std::make_shared<x::LocationService>(topology, locationIndex);

    monitoringService = std::make_shared<MonitoringService>(topology, queryService, queryCatalogService, enableMonitoring);
    monitoringService->getMonitoringManager()->registerLogicalMonitoringStreams(this->coordinatorConfiguration);
}

xCoordinator::~xCoordinator() {
    stopCoordinator(true);
    x_DEBUG("xCoordinator::~xCoordinator() map cleared");
    sourceCatalog->reset();
    queryCatalog->clearQueries();
}

xWorkerPtr xCoordinator::getxWorker() { return worker; }

Runtime::NodeEnginePtr xCoordinator::getNodeEngine() { return worker->getNodeEngine(); }
bool xCoordinator::isCoordinatorRunning() { return isRunning; }

uint64_t xCoordinator::startCoordinator(bool blocking) {
    x_DEBUG("xCoordinator start");

    auto expected = false;
    if (!isRunning.compare_exchange_strong(expected, true)) {
        x_ASSERT2_FMT(false, "cannot start x coordinator");
    }

    queryRequestProcessorThread = std::make_shared<std::thread>(([&]() {
        setThreadName("RqstProc");

        x_INFO("xCoordinator: started queryRequestProcessor");
        queryRequestProcessorService->start();
        x_WARNING("xCoordinator: finished queryRequestProcessor");
    }));

    x_DEBUG("xCoordinator: startCoordinatorRPCServer: Building GRPC Server");
    std::shared_ptr<std::promise<bool>> promRPC = std::make_shared<std::promise<bool>>();

    rpcThread = std::make_shared<std::thread>(([this, promRPC]() {
        setThreadName("xRPC");

        x_DEBUG("xCoordinator: buildAndStartGRPCServer");
        buildAndStartGRPCServer(promRPC);
        x_DEBUG("xCoordinator: buildAndStartGRPCServer: end listening");
    }));
    promRPC->get_future().get();
    x_DEBUG("xCoordinator:buildAndStartGRPCServer: ready");

    x_DEBUG("xCoordinator: Register Logical sources");
    for (auto logicalSource : coordinatorConfiguration->logicalSources.getValues()) {
        sourceCatalogService->registerLogicalSource(logicalSource.getValue()->getLogicalSourceName(),
                                                    logicalSource.getValue()->getSchema());
    }
    x_DEBUG("xCoordinator: Finished Registering Logical source");

    //start the coordinator worker that is the sink for all queryIdAndCatalogEntryMapping
    x_DEBUG("xCoordinator::startCoordinator: start x worker");
    // Unconditionally set IP of internal worker and set IP and port of coordinator.
    coordinatorConfiguration->worker.coordinatorIp = rpcIp;
    coordinatorConfiguration->worker.coordinatorPort = rpcPort;
    coordinatorConfiguration->worker.localWorkerIp = rpcIp;
    // Ensure that coordinator and internal worker enable/disable monitoring together.
    coordinatorConfiguration->worker.enableMonitoring = enableMonitoring;
    // Create a copy of the worker configuration to pass to the xWorker.
    auto workerConfig = std::make_shared<WorkerConfiguration>(coordinatorConfiguration->worker);
    worker = std::make_shared<xWorker>(std::move(workerConfig), monitoringService->getMonitoringManager()->getMetricStore());
    worker->start(/**blocking*/ false, /**withConnect*/ true);

    x::Exceptions::installGlobalErrorListener(worker);

    //Start rest that accepts queryIdAndCatalogEntryMapping form the outsides
    x_DEBUG("xCoordinator starting rest server");

    //setting the allowed origins for http request to the rest server
    std::optional<std::string> allowedOrigin = std::nullopt;
    auto originString = coordinatorConfiguration->restServerCorsAllowedOrigin.getValue();
    if (!originString.empty()) {
        x_INFO("CORS: allow origin: {}", originString);
        allowedOrigin = originString;
    }

    restServer = std::make_shared<RestServer>(restIp,
                                              restPort,
                                              this->inherited0::weak_from_this(),
                                              queryCatalogService,
                                              sourceCatalogService,
                                              topologyManagerService,
                                              globalExecutionPlan,
                                              queryService,
                                              monitoringService,
                                              globalQueryPlan,
                                              udfCatalog,
                                              worker->getNodeEngine()->getBufferManager(),
                                              locationService,
                                              allowedOrigin);
    restThread = std::make_shared<std::thread>(([&]() {
        setThreadName("xREST");
        restServer->start();//this call is blocking
    }));

    x_DEBUG("xCoordinator::startCoordinatorRESTServer: ready");

    healthCheckService =
        std::make_shared<CoordinatorHealthCheckService>(topologyManagerService, HEALTH_SERVICE_NAME, coordinatorConfiguration);
    topologyManagerService->setHealthService(healthCheckService);
    x_DEBUG("xCoordinator start health check");
    healthCheckService->startHealthCheck();

    if (blocking) {//blocking is for the starter to wait here for user to send query
        x_DEBUG("xCoordinator started, join now and waiting for work");
        restThread->join();
        x_DEBUG("xCoordinator Required stopping");
    } else {//non-blocking is used for tests to continue execution
        x_DEBUG("xCoordinator started, return without blocking on port {}", rpcPort);
        return rpcPort;
    }
    return 0UL;
}

Catalogs::Source::SourceCatalogPtr xCoordinator::getSourceCatalog() const { return sourceCatalog; }

ReplicationServicePtr xCoordinator::getReplicationService() const { return replicationService; }

TopologyPtr xCoordinator::getTopology() const { return topology; }

bool xCoordinator::stopCoordinator(bool force) {
    x_DEBUG("xCoordinator: stopCoordinator force={}", force);
    auto expected = true;
    if (isRunning.compare_exchange_strong(expected, false)) {

        x_DEBUG("xCoordinator::stop health check");
        healthCheckService->stopHealthCheck();

        bool successShutdownWorker = worker->stop(force);
        if (!successShutdownWorker) {
            x_ERROR("xCoordinator::stop node engine stop not successful");
            x_THROW_RUNTIME_ERROR("xCoordinator::stop error while stopping node engine");
        }
        x_DEBUG("xCoordinator::stop Node engine stopped successfully");

        x_DEBUG("xCoordinator: stopping rest server");
        bool successStopRest = restServer->stop();
        if (!successStopRest) {
            x_ERROR("xCoordinator::stopCoordinator: error while stopping restServer");
            x_THROW_RUNTIME_ERROR("Error while stopping xCoordinator");
        }
        x_DEBUG("xCoordinator: rest server stopped {}", successStopRest);

        if (restThread->joinable()) {
            x_DEBUG("xCoordinator: join restThread");
            restThread->join();
        } else {
            x_ERROR("xCoordinator: rest thread not joinable");
            x_THROW_RUNTIME_ERROR("Error while stopping thread->join");
        }

        queryRequestProcessorService->shutDown();
        if (queryRequestProcessorThread->joinable()) {
            x_DEBUG("xCoordinator: join queryRequestProcessorThread");
            queryRequestProcessorThread->join();
            x_DEBUG("xCoordinator: joined queryRequestProcessorThread");
        } else {
            x_ERROR("xCoordinator: query processor thread not joinable");
            x_THROW_RUNTIME_ERROR("Error while stopping thread->join");
        }

        x_DEBUG("xCoordinator: stopping rpc server");
        rpcServer->Shutdown();
        rpcServer->Wait();

        if (rpcThread->joinable()) {
            x_DEBUG("xCoordinator: join rpcThread");
            rpcThread->join();
            rpcThread.reset();

        } else {
            x_ERROR("xCoordinator: rpc thread not joinable");
            x_THROW_RUNTIME_ERROR("Error while stopping thread->join");
        }

        return true;
    }
    x_DEBUG("xCoordinator: already stopped");
    return true;
}

void xCoordinator::buildAndStartGRPCServer(const std::shared_ptr<std::promise<bool>>& prom) {
    grpc::ServerBuilder builder;
    x_ASSERT(sourceCatalogService, "null sourceCatalogService");
    x_ASSERT(topologyManagerService, "null topologyManagerService");
    this->replicationService = std::make_shared<ReplicationService>(this->inherited0::shared_from_this());
    CoordinatorRPCServer service(queryService,
                                 topologyManagerService,
                                 sourceCatalogService,
                                 queryCatalogService,
                                 monitoringService->getMonitoringManager(),
                                 this->replicationService,
                                 locationService);

    std::string address = rpcIp + ":" + std::to_string(rpcPort);
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::HealthCheckServiceInterface> healthCheckServiceInterface;
    std::unique_ptr<grpc::ServerBuilderOption> option(
        new grpc::HealthCheckServiceServerBuilderOption(std::move(healthCheckServiceInterface)));
    builder.SetOption(std::move(option));
    HealthCheckRPCServer healthCheckServiceImpl;
    healthCheckServiceImpl.SetStatus(
        HEALTH_SERVICE_NAME,
        grpc::health::v1::HealthCheckResponse_ServingStatus::HealthCheckResponse_ServingStatus_SERVING);
    builder.RegisterService(&healthCheckServiceImpl);

    rpcServer = builder.BuildAndStart();
    prom->set_value(true);
    x_DEBUG("xCoordinator: buildAndStartGRPCServerServer listening on address={}", address);
    rpcServer->Wait();//blocking call
    x_DEBUG("xCoordinator: buildAndStartGRPCServer end listening");
}

std::vector<Runtime::QueryStatisticsPtr> xCoordinator::getQueryStatistics(QueryId queryId) {
    x_INFO("xCoordinator: Get query statistics for query Id {}", queryId);
    return worker->getNodeEngine()->getQueryStatistics(queryId);
}

QueryServicePtr xCoordinator::getQueryService() { return queryService; }

QueryCatalogServicePtr xCoordinator::getQueryCatalogService() { return queryCatalogService; }

Catalogs::UDF::UDFCatalogPtr xCoordinator::getUDFCatalog() { return udfCatalog; }

MonitoringServicePtr xCoordinator::getMonitoringService() { return monitoringService; }

GlobalQueryPlanPtr xCoordinator::getGlobalQueryPlan() { return globalQueryPlan; }

void xCoordinator::onFatalError(int, std::string) {}

void xCoordinator::onFatalException(const std::shared_ptr<std::exception>, std::string) {}

SourceCatalogServicePtr xCoordinator::getSourceCatalogService() const { return sourceCatalogService; }

TopologyManagerServicePtr xCoordinator::getTopologyManagerService() const { return topologyManagerService; }

LocationServicePtr xCoordinator::getLocationService() const { return locationService; }

GlobalExecutionPlanPtr xCoordinator::getGlobalExecutionPlan() const { return globalExecutionPlan; }

}// namespace x
