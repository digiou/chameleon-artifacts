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
#include <Components/xWorker.hpp>
#include <Configurations/WorkerConfigurationKeys.hpp>
#include <CoordinatorRPCService.pb.h>
#include <GRPC/CallData.hpp>
#include <GRPC/CoordinatorRPCClient.hpp>
#include <GRPC/HealthCheckRPCServer.hpp>
#include <GRPC/WorkerRPCServer.hpp>
#include <Monitoring/Metrics/Gauge/RegistrationMetrics.hpp>
#include <Monitoring/MonitoringAgent.hpp>
#include <Monitoring/MonitoringPlan.hpp>
#include <Monitoring/Storage/AbstractMetricStore.hpp>
#include <Network/NetworkManager.hpp>
#include <Runtime/NodeEngine.hpp>
#include <Runtime/NodeEngineBuilder.hpp>
#include <Runtime/QueryStatistics.hpp>
#include <Services/WorkerHealthCheckService.hpp>
#include <Spatial/DataTypes/Waypoint.hpp>
#include <Spatial/Mobility/LocationProviders/LocationProvider.hpp>
#include <Spatial/Mobility/ReconnectSchedulePredictors/ReconnectSchedule.hpp>
#include <Spatial/Mobility/ReconnectSchedulePredictors/ReconnectSchedulePredictor.hpp>
#include <Spatial/Mobility/WorkerMobilityHandler.hpp>
#include <Util/Experimental/SpatialTypeUtility.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadNaming.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <csignal>
#include <future>
#include <grpcpp/ext/health_check_service_server_builder_option.h>
#include <grpcpp/health_check_service_interface.h>
#include <iomanip>

#include <utility>

using namespace std;
volatile sig_atomic_t flag = 0;

void termFunc(int) {
    cout << "termfunc" << endl;
    flag = 1;
}

namespace x {

xWorker::xWorker(Configurations::WorkerConfigurationPtr&& workerConfig, Monitoring::MetricStorePtr metricStore)
    : workerConfig(workerConfig), localWorkerRpcPort(workerConfig->rpcPort), workerId(INVALID_TOPOLOGY_NODE_ID),
      metricStore(metricStore), parentId(workerConfig->parentId),
      mobilityConfig(std::make_shared<x::Configurations::Spatial::Mobility::Experimental::WorkerMobilityConfiguration>(
          workerConfig->mobilityConfiguration)) {
    setThreadName("xWorker");
    x_DEBUG("xWorker: constructed");
    x_ASSERT2_FMT(workerConfig->coordinatorPort > 0, "Cannot use 0 as coordinator port");
    rpcAddress = workerConfig->localWorkerIp.getValue() + ":" + std::to_string(localWorkerRpcPort);
}

xWorker::~xWorker() { stop(true); }

void xWorker::handleRpcs(WorkerRPCServer& service) {
    //TODO: somehow we need this although it is not called at all
    // Spawn a new CallData instance to serve new clients.

    CallData call(service, completionQueue.get());
    call.proceed();
    void* tag = nullptr;// uniquely identifies a request.
    bool ok = 0;        //
    while (true) {
        // Block waiting to read the next event from the completion queue. The
        // event is uniquely identified by its tag, which in this case is the
        // memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or completionQueue is shutting down.
        bool ret = completionQueue->Next(&tag, &ok);
        x_DEBUG("handleRpcs got item from queue with ret={}", ret);
        if (!ret) {
            //we are going to shut down
            return;
        }
        x_ASSERT(ok, "handleRpcs got invalid message");
        static_cast<CallData*>(tag)->proceed();
    }
}

void xWorker::buildAndStartGRPCServer(const std::shared_ptr<std::promise<int>>& portPromise) {
    WorkerRPCServer service(nodeEngine, monitoringAgent, locationProvider, trajectoryPredictor);
    ServerBuilder builder;
    int actualRpcPort;
    builder.AddListeningPort(rpcAddress, grpc::InsecureServerCredentials(), &actualRpcPort);
    builder.RegisterService(&service);
    completionQueue = builder.AddCompletionQueue();

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
    portPromise->set_value(actualRpcPort);
    x_DEBUG("xWorker: buildAndStartGRPCServer Server listening on address {}: {}", rpcAddress, actualRpcPort);
    //this call is already blocking
    handleRpcs(service);

    rpcServer->Wait();
    x_DEBUG("xWorker: buildAndStartGRPCServer end listening");
}

uint64_t xWorker::getWorkerId() { return coordinatorRpcClient->getId(); }

bool xWorker::start(bool blocking, bool withConnect) {
    x_DEBUG("xWorker: start with blocking {} workerId={} coordinatorIp={} coordinatorPort={} localWorkerIp={} "
              "localWorkerRpcPort={} "
              "localWorkerZmqPort={} windowStrategy={}",
              blocking,
              workerConfig->workerId.getValue(),
              workerConfig->coordinatorIp.getValue(),
              workerConfig->coordinatorPort.getValue(),
              workerConfig->localWorkerIp.getValue(),
              localWorkerRpcPort,
              workerConfig->dataPort.getValue(),
              magic_enum::enum_name(workerConfig->queryCompiler.windowingStrategy.getValue()));

    x_DEBUG("xWorker::start: start Runtime");
    auto expected = false;
    if (!isRunning.compare_exchange_strong(expected, true)) {
        x_ASSERT2_FMT(false, "cannot start x worker");
    }

    try {
        x_DEBUG("xWorker: MonitoringAgent configured with monitoring={}", workerConfig->enableMonitoring.getValue());
        monitoringAgent = Monitoring::MonitoringAgent::create(workerConfig->enableMonitoring.getValue());
        monitoringAgent->addMonitoringStreams(workerConfig);

        nodeEngine =
            Runtime::NodeEngineBuilder::create(workerConfig).setQueryStatusListener(this->inherited0::shared_from_this()).build();
        if (metricStore != nullptr) {
            nodeEngine->setMetricStore(metricStore);
        }
        x_DEBUG("xWorker: Node engine started successfully");
    } catch (std::exception& err) {
        x_ERROR("xWorker: node engine could not be started");
        throw Exceptions::RuntimeException("xWorker error while starting node engine");
    }

    x_DEBUG("xWorker: request startWorkerRPCServer for accepting messages for address={}: {}",
              rpcAddress,
              localWorkerRpcPort.load());
    std::shared_ptr<std::promise<int>> promRPC = std::make_shared<std::promise<int>>();

    if (workerConfig->nodeSpatialType.getValue() != x::Spatial::Experimental::SpatialType::NO_LOCATION) {
        locationProvider = x::Spatial::Mobility::Experimental::LocationProvider::create(workerConfig);
        if (locationProvider->getSpatialType() == x::Spatial::Experimental::SpatialType::MOBILE_NODE) {
            //is s2 is activated, create a reconnect schedule predictor
            trajectoryPredictor = x::Spatial::Mobility::Experimental::ReconnectSchedulePredictor::create(mobilityConfig);
        }
    }

    rpcThread = std::make_shared<std::thread>(([this, promRPC]() {
        x_DEBUG("xWorker: buildAndStartGRPCServer");
        buildAndStartGRPCServer(promRPC);
        x_DEBUG("xWorker: buildAndStartGRPCServer: end listening");
    }));
    localWorkerRpcPort.store(promRPC->get_future().get());
    rpcAddress = workerConfig->localWorkerIp.getValue() + ":" + std::to_string(localWorkerRpcPort.load());
    x_DEBUG("xWorker: startWorkerRPCServer ready for accepting messages for address={}: {}",
              rpcAddress,
              localWorkerRpcPort.load());

    if (withConnect) {
        x_DEBUG("xWorker: start with connect");
        bool con = connect();

        x_ASSERT(con, "cannot connect");
    }

    if (parentId > xCoordinator::x_COORDINATOR_ID) {
        x_DEBUG("xWorker: add parent id={}", parentId);
        bool success = replaceParent(xCoordinator::x_COORDINATOR_ID, parentId);
        x_DEBUG("parent add= {}", success);
        x_ASSERT(success, "cannot addParent");
    }

    if (withConnect && locationProvider
        && locationProvider->getSpatialType() == x::Spatial::Experimental::SpatialType::MOBILE_NODE) {
        workerMobilityHandler =
            std::make_shared<x::Spatial::Mobility::Experimental::WorkerMobilityHandler>(locationProvider,
                                                                                          coordinatorRpcClient,
                                                                                          nodeEngine,
                                                                                          mobilityConfig);
        //FIXME: currently the worker mobility handler will only work with exactly one parent
        auto parentIds = coordinatorRpcClient->getParents(workerId);
        if (parentIds.size() > 1) {
            x_WARNING("Attempting to start worker mobility handler for worker with multiple parents. This is"
                        "currently not supported, mobility handler will not be started");
        } else {
            workerMobilityHandler->start(parentIds);
        }
    }

    if (workerConfig->enableStatisticOuput) {
        statisticOutputThread = std::make_shared<std::thread>(([this]() {
            x_DEBUG("xWorker: start statistic collection");
            while (isRunning) {
                auto ts = std::chrono::system_clock::now();
                auto timeNow = std::chrono::system_clock::to_time_t(ts);
                auto stats = nodeEngine->getQueryStatistics(true);
                for (auto& query : stats) {
                    std::cout << "Statistics " << std::put_time(std::localtime(&timeNow), "%Y-%m-%d %X") << " =>"
                              << query.getQueryStatisticsAsString() << std::endl;
                }
                sleep(1);
            }
            x_DEBUG("xWorker: statistic collection end");
        }));
    }
    if (blocking) {
        x_DEBUG("xWorker: started, join now and waiting for work");
        signal(SIGINT, termFunc);
        while (true) {
            if (flag) {
                x_DEBUG("xWorker: caught signal terminating worker");
                flag = 0;
                break;
            }
            //cout << "xWorker wait" << endl;
            sleep(5);
        }
    }

    x_DEBUG("xWorker: started, return");
    return true;
}

Runtime::NodeEnginePtr xWorker::getNodeEngine() { return nodeEngine; }

bool xWorker::isWorkerRunning() const noexcept { return isRunning; }

bool xWorker::stop(bool) {
    x_DEBUG("xWorker: stop");

    auto expected = true;
    if (isRunning.compare_exchange_strong(expected, false)) {
        x_DEBUG("xWorker::stopping health check");
        if (healthCheckService) {
            healthCheckService->stopHealthCheck();
        } else {
            x_WARNING("No health check service was created");
        }

        if (workerMobilityHandler) {
            workerMobilityHandler->stop();
            x_TRACE("triggered stopping of location update push thread");
        }
        bool successShutdownNodeEngine = nodeEngine->stop();
        if (!successShutdownNodeEngine) {
            x_ERROR("xWorker::stop node engine stop not successful");
            x_THROW_RUNTIME_ERROR("xWorker::stop  error while stopping node engine");
        }
        x_DEBUG("xWorker::stop : Node engine stopped successfully");
        nodeEngine.reset();

        x_DEBUG("xWorker: stopping rpc server");
        rpcServer->Shutdown();
        //shut down the async queue
        completionQueue->Shutdown();

        if (rpcThread->joinable()) {
            x_DEBUG("xWorker: join rpcThread");
            rpcThread->join();
        }

        rpcServer.reset();
        rpcThread.reset();
        if (statisticOutputThread && statisticOutputThread->joinable()) {
            x_DEBUG("xWorker: statistic collection thread join");
            statisticOutputThread->join();
        }
        statisticOutputThread.reset();

        return successShutdownNodeEngine;
    }
    x_WARNING("xWorker::stop: already stopped");
    return true;
}

bool xWorker::connect() {

    std::string coordinatorAddress = workerConfig->coordinatorIp.getValue() + ":" + std::to_string(workerConfig->coordinatorPort);
    x_DEBUG("xWorker::connect() Registering worker with coordinator at {}", coordinatorAddress);
    coordinatorRpcClient = std::make_shared<CoordinatorRPCClient>(coordinatorAddress);

    RegisterWorkerRequest registrationRequest;
    registrationRequest.set_workerid(workerConfig->workerId.getValue());
    registrationRequest.set_address(workerConfig->localWorkerIp.getValue());
    registrationRequest.set_grpcport(localWorkerRpcPort.load());
    registrationRequest.set_dataport(nodeEngine->getNetworkManager()->getServerDataPort());
    registrationRequest.set_numberofslots(workerConfig->numberOfSlots.getValue());
    registrationRequest.mutable_registrationmetrics()->Swap(monitoringAgent->getRegistrationMetrics().serialize().get());
    //todo: what about this?
    registrationRequest.set_javaudfsupported(workerConfig->isJavaUDFSupported.getValue());
    registrationRequest.set_spatialtype(
        x::Spatial::Util::SpatialTypeUtility::toProtobufEnum(workerConfig->nodeSpatialType.getValue()));

    if (locationProvider) {
        auto waypoint = registrationRequest.mutable_waypoint();
        auto currentWaypoint = locationProvider->getCurrentWaypoint();
        if (currentWaypoint.getTimestamp()) {
            waypoint->set_timestamp(currentWaypoint.getTimestamp().value());
        }
        auto geolocation = waypoint->mutable_geolocation();
        geolocation->set_lat(currentWaypoint.getLocation().getLatitude());
        geolocation->set_lng(currentWaypoint.getLocation().getLongitude());
    }

    bool successPRCRegister = coordinatorRpcClient->registerWorker(registrationRequest);

    x_DEBUG("xWorker::connect() Worker registered successfully and got id={}", coordinatorRpcClient->getId());
    workerId = coordinatorRpcClient->getId();
    monitoringAgent->setNodeId(workerId);
    if (successPRCRegister) {
        if (workerId != workerConfig->workerId) {
            if (workerConfig->workerId == INVALID_TOPOLOGY_NODE_ID) {
                // workerId value is written in the yaml for the first time
                x_DEBUG("xWorker::connect() Persisting workerId={} in yaml file for the first time.", workerId);
                bool success =
                    getWorkerConfiguration()->persistWorkerIdInYamlConfigFile(workerConfig->configPath, workerId, false);
                if (!success) {
                    x_WARNING("xWorker::connect() Could not persist workerId in yaml config file");
                } else {
                    x_DEBUG("xWorker::connect() Persisted workerId={} successfully in yaml file.", workerId);
                }
            } else {
                // a value was in the yaml file but it's being overwritten, because the coordinator assigns a new value
                x_DEBUG("xWorker::connect() Coordinator assigned new workerId value. Persisting workerId={} in yaml file",
                          workerId);
                bool success =
                    getWorkerConfiguration()->persistWorkerIdInYamlConfigFile(workerConfig->configPath, workerId, true);
                if (!success) {
                    x_WARNING("xWorker::connect() Could not persist workerId in yaml config file");
                } else {
                    x_DEBUG("xWorker::connect() Persisted workerId={} successfully in yaml file.", workerId);
                }
            }
        }
        x_DEBUG("xWorker::registerWorker rpc register success with id {}", workerId);
        connected = true;
        nodeEngine->setNodeId(workerId);
        healthCheckService = std::make_shared<WorkerHealthCheckService>(coordinatorRpcClient,
                                                                        HEALTH_SERVICE_NAME,
                                                                        this->inherited0::shared_from_this());
        x_DEBUG("xWorker start health check");
        healthCheckService->startHealthCheck();

        auto configPhysicalSources = workerConfig->physicalSources.getValues();
        if (!configPhysicalSources.empty()) {
            std::vector<PhysicalSourcePtr> physicalSources;
            for (auto& physicalSource : configPhysicalSources) {
                physicalSources.push_back(physicalSource);
            }
            x_DEBUG("xWorker: start with register source");
            bool success = registerPhysicalSources(physicalSources);
            x_DEBUG("registered= {}", success);
            x_ASSERT(success, "cannot register");
        }
        return true;
    }
    x_DEBUG("xWorker::registerWorker rpc register failed");
    connected = false;
    return connected;
}

bool xWorker::disconnect() {
    x_DEBUG("xWorker::disconnect()");
    bool successPRCRegister = coordinatorRpcClient->unregisterNode();
    if (successPRCRegister) {
        x_DEBUG("xWorker::registerWorker rpc unregister success");
        connected = false;
        x_DEBUG("xWorker::stop health check");
        healthCheckService->stopHealthCheck();
        x_DEBUG("xWorker::stop health check successful");
        return true;
    }
    x_DEBUG("xWorker::registerWorker rpc unregister failed");
    return false;
}

bool xWorker::unregisterPhysicalSource(std::string logicalName, std::string physicalName) {
    bool success = coordinatorRpcClient->unregisterPhysicalSource(std::move(logicalName), std::move(physicalName));
    x_DEBUG("xWorker::unregisterPhysicalSource success={}", success);
    return success;
}

const Configurations::WorkerConfigurationPtr& xWorker::getWorkerConfiguration() const { return workerConfig; }

bool xWorker::registerPhysicalSources(const std::vector<PhysicalSourcePtr>& physicalSources) {
    x_ASSERT(!physicalSources.empty(), "invalid physical sources");
    bool con = waitForConnect();

    x_ASSERT(con, "cannot connect");
    bool success = coordinatorRpcClient->registerPhysicalSources(physicalSources);
    x_ASSERT(success, "failed to register source");
    x_DEBUG("xWorker::registerPhysicalSources success={}", success);
    return success;
}

bool xWorker::addParent(uint64_t parentId) {
    bool con = waitForConnect();

    x_ASSERT(con, "Connection failed");
    bool success = coordinatorRpcClient->addParent(parentId);
    x_DEBUG("xWorker::addNewLink(parent only) success={}", success);
    return success;
}

bool xWorker::replaceParent(uint64_t oldParentId, uint64_t newParentId) {
    bool con = waitForConnect();

    x_ASSERT(con, "Connection failed");
    bool success = coordinatorRpcClient->replaceParent(oldParentId, newParentId);
    if (!success) {
        x_WARNING("xWorker::replaceParent() failed to replace oldParent={} with newParentId={}", oldParentId, newParentId);
    }
    x_DEBUG("xWorker::replaceParent() success={}", success);
    return success;
}

bool xWorker::removeParent(uint64_t parentId) {
    bool con = waitForConnect();

    x_ASSERT(con, "Connection failed");
    bool success = coordinatorRpcClient->removeParent(parentId);
    x_DEBUG("xWorker::removeLink(parent only) success={}", success);
    return success;
}

std::vector<Runtime::QueryStatisticsPtr> xWorker::getQueryStatistics(QueryId queryId) {
    return nodeEngine->getQueryStatistics(queryId);
}

bool xWorker::waitForConnect() const {
    x_DEBUG("xWorker::waitForConnect()");
    auto timeoutInSec = std::chrono::seconds(3);
    auto start_timestamp = std::chrono::system_clock::now();
    while (std::chrono::system_clock::now() < start_timestamp + timeoutInSec) {
        x_DEBUG("waitForConnect: check connect");
        if (!connected) {
            x_DEBUG("waitForConnect: not connected, sleep");
            sleep(1);
        } else {
            x_DEBUG("waitForConnect: connected");
            return true;
        }
    }
    x_DEBUG("waitForConnect: not connected after timeout");
    return false;
}

bool xWorker::notifyQueryStatusChange(QueryId queryId,
                                        QuerySubPlanId subQueryId,
                                        Runtime::Execution::ExecutableQueryPlanStatus newStatus) {
    x_ASSERT(waitForConnect(), "cannot connect");
    x_ASSERT2_FMT(newStatus != Runtime::Execution::ExecutableQueryPlanStatus::Stopped,
                    "Hard Stop called for query=" << queryId << " subQueryId=" << subQueryId
                                                  << " should not call notifyQueryStatusChange");
    if (newStatus == Runtime::Execution::ExecutableQueryPlanStatus::Finished) {
        x_DEBUG("xWorker {} about to notify soft stop completion for query {} subPlan {}",
                  getWorkerId(),
                  queryId,
                  subQueryId);
        return coordinatorRpcClient->notifySoftStopCompleted(queryId, subQueryId);
    } else if (newStatus == Runtime::Execution::ExecutableQueryPlanStatus::ErrorState) {
        return true;// rpc to coordinator executed from async runner
    }
    return false;
}

bool xWorker::canTriggerEndOfStream(QueryId queryId,
                                      QuerySubPlanId subPlanId,
                                      OperatorId sourceId,
                                      Runtime::QueryTerminationType terminationType) {
    x_ASSERT(waitForConnect(), "cannot connect");
    x_ASSERT(terminationType == Runtime::QueryTerminationType::Graceful, "invalid termination type");
    return coordinatorRpcClient->checkAndMarkForSoftStop(queryId, subPlanId, sourceId);
}

bool xWorker::notifySourceTermination(QueryId queryId,
                                        QuerySubPlanId subPlanId,
                                        OperatorId sourceId,
                                        Runtime::QueryTerminationType queryTermination) {
    x_ASSERT(waitForConnect(), "cannot connect");
    return coordinatorRpcClient->notifySourceStopTriggered(queryId, subPlanId, sourceId, queryTermination);
}

bool xWorker::notifyQueryFailure(uint64_t queryId, uint64_t subQueryId, std::string errorMsg) {
    bool con = waitForConnect();
    x_ASSERT(con, "Connection failed");
    bool success = coordinatorRpcClient->notifyQueryFailure(queryId, subQueryId, getWorkerId(), 0, errorMsg);
    x_DEBUG("xWorker::notifyQueryFailure success={}", success);
    return success;
}

bool xWorker::notifyEpochTermination(uint64_t timestamp, uint64_t queryId) {
    bool con = waitForConnect();
    x_ASSERT(con, "Connection failed");
    bool success = coordinatorRpcClient->notifyEpochTermination(timestamp, queryId);
    x_DEBUG("xWorker::propagatePunctuation success={}", success);
    return success;
}

bool xWorker::notifyErrors(uint64_t workerId, std::string errorMsg) {
    bool con = waitForConnect();
    x_ASSERT(con, "Connection failed");
    x_DEBUG("xWorker::sendErrors worker {} going to send error={}", workerId, errorMsg);
    bool success = coordinatorRpcClient->sendErrors(workerId, errorMsg);
    x_DEBUG("xWorker::sendErrors success={}", success);
    return success;
}

void xWorker::onFatalError(int signalNumber, std::string callstack) {
    x_ERROR("onFatalError: signal [{}] error [{}] callstack {} ", signalNumber, strerror(errno), callstack);
    std::string errorMsg;
    std::cerr << "xWorker failed fatally" << std::endl;// it's necessary for testing and it wont harm us to write to stderr
    std::cerr << "Error: " << strerror(errno) << std::endl;
    std::cerr << "Signal: " << std::to_string(signalNumber) << std::endl;
    std::cerr << "Callstack:\n " << callstack << std::endl;
    // save errors in errorMsg
    errorMsg =
        "onFatalError: signal [" + std::to_string(signalNumber) + "] error [" + strerror(errno) + "] callstack " + callstack;
    //send it to Coordinator
    notifyErrors(getWorkerId(), errorMsg);
#ifdef ENABLE_CORE_DUMPER
    detail::createCoreDump();
#endif
}

void xWorker::onFatalException(std::shared_ptr<std::exception> ptr, std::string callstack) {
    x_ERROR("onFatalException: exception=[{}] callstack={}", ptr->what(), callstack);
    std::string errorMsg;
    std::cerr << "xWorker failed fatally" << std::endl;
    std::cerr << "Error: " << strerror(errno) << std::endl;
    std::cerr << "Exception: " << ptr->what() << std::endl;
    std::cerr << "Callstack:\n " << callstack << std::endl;
    // save errors in errorMsg
    errorMsg = "onFatalException: exception=[" + std::string(ptr->what()) + "] callstack=\n" + callstack;
    //send it to Coordinator
    this->notifyErrors(this->getWorkerId(), errorMsg);
#ifdef ENABLE_CORE_DUMPER
    detail::createCoreDump();
#endif
}

TopologyNodeId xWorker::getTopologyNodeId() const { return workerId; }

x::Spatial::Mobility::Experimental::LocationProviderPtr xWorker::getLocationProvider() { return locationProvider; }

x::Spatial::Mobility::Experimental::ReconnectSchedulePredictorPtr xWorker::getTrajectoryPredictor() {
    return trajectoryPredictor;
}

x::Spatial::Mobility::Experimental::WorkerMobilityHandlerPtr xWorker::getMobilityHandler() { return workerMobilityHandler; }

}// namespace x