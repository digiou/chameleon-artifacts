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

#ifndef x_CORE_INCLUDE_COMPONENTS_xWORKER_HPP_
#define x_CORE_INCLUDE_COMPONENTS_xWORKER_HPP_

#include <Common/Identifiers.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Exceptions/ErrorListener.hpp>
#include <Listeners/QueryStatusListener.hpp>
#include <Runtime/RuntimeForwardRefs.hpp>
#include <Services/ReplicationService.hpp>
#include <future>
#include <memory>
#include <optional>
#include <vector>

namespace grpc {
class Server;
class ServerCompletionQueue;
};// namespace grpc

namespace x {

namespace Spatial::Index::Experimental {
class Location;
using LocationPtr = std::shared_ptr<Location>;
}// namespace Spatial::Index::Experimental

namespace Spatial::Mobility::Experimental {
class LocationProvider;
using LocationProviderPtr = std::shared_ptr<LocationProvider>;

class ReconnectSchedulePredictor;
using ReconnectSchedulePredictorPtr = std::shared_ptr<ReconnectSchedulePredictor>;

class WorkerMobilityHandler;
using WorkerMobilityHandlerPtr = std::shared_ptr<WorkerMobilityHandler>;

}// namespace Spatial::Mobility::Experimental

namespace Configurations::Spatial::Mobility::Experimental {
class WorkerMobilityConfiguration;
using WorkerMobilityConfigurationPtr = std::shared_ptr<WorkerMobilityConfiguration>;
}// namespace Configurations::Spatial::Mobility::Experimental

class WorkerRPCServer;
class CoordinatorRPCClient;
using CoordinatorRPCClientPtr = std::shared_ptr<CoordinatorRPCClient>;

namespace Monitoring {
class MonitoringAgent;
using MonitoringAgentPtr = std::shared_ptr<MonitoringAgent>;

class AbstractMetricStore;
using MetricStorePtr = std::shared_ptr<AbstractMetricStore>;
}// namespace Monitoring

static constexpr auto HEALTH_SERVICE_NAME = "x_DEFAULT_HEALTH_CHECK_SERVICE";

class xWorker : public detail::virtual_enable_shared_from_this<xWorker>,
                  public Exceptions::ErrorListener,
                  public AbstractQueryStatusListener {
    using inherited0 = detail::virtual_enable_shared_from_this<xWorker>;
    using inherited1 = ErrorListener;

  public:
    /**
     * @brief default constructor which creates a sensor node with a metric store
     * @note this will create the worker actor using the default worker config
     */
    xWorker(Configurations::WorkerConfigurationPtr&& workerConfig, Monitoring::MetricStorePtr metricStore = nullptr);

    /**
     * @brief default dtor
     */
    ~xWorker();

    /**
     * @brief start the worker using the default worker config
     * @param blocking: bool indicating if the call is blocking
     * @param withConnect: bool indicating if connect
     * @return bool indicating success
     */
    bool start(bool blocking, bool withConnect);

    /**
     * @brief stop the worker
     * @return bool indicating success
     */
    bool stop(bool force);

    /**
     * @brief connect to coordinator using the default worker config
     * @return bool indicating success
     */
    bool connect();

    /**
     * @brief disconnect from coordinator
     * @return bool indicating success
     */
    bool disconnect();

    /**
    * @brief method to deregister physical source with the coordinator
    * @param logical and physical of the source
     * @return bool indicating success
    */
    bool unregisterPhysicalSource(std::string logicalName, std::string physicalName);

    /**
    * @brief method add new parent to this node
    * @param parentId
    * @return bool indicating success
    */
    bool addParent(uint64_t parentId);

    /**
    * @brief method to replace old with new parent
    * @param oldParentId
    * @param newParentId
    * @return bool indicating success
    */
    bool replaceParent(uint64_t oldParentId, uint64_t newParentId);

    /**
    * @brief method remove parent from this node
    * @param parentId
    * @return bool indicating success
    */
    bool removeParent(uint64_t parentId);

    /**
    * @brief method to return the query statistics
    * @param id of the query
    * @return vector of queryStatistics
    */
    std::vector<Runtime::QueryStatisticsPtr> getQueryStatistics(QueryId queryId);

    /**
     * @brief method to get a ptr to the node engine
     * @return pt to node engine
     */
    Runtime::NodeEnginePtr getNodeEngine();

    /**
     * @brief method to get the id of the worker
     * @return id of the worker
     */
    TopologyNodeId getTopologyNodeId() const;

    /**
     * @brief Method to check if a worker is still running
     * @return running status of the worker
     */
    [[nodiscard]] bool isWorkerRunning() const noexcept;

    /**
     * @brief Method to let the Coordinator know that a Query failed
     * @param queryId of the failed query
     * @param subQueryId of the failed query
     * @param workerId of the worker that handled the failed query
     * @param operatorId of failed query
     * @param errorMsg to describe the reason of the failure
     * @return true if Notification was successful, false otherwise
     */
    bool notifyQueryFailure(QueryId queryId, QuerySubPlanId subQueryId, std::string errorMsg) override;

    bool notifySourceTermination(QueryId queryId,
                                 QuerySubPlanId subPlanId,
                                 OperatorId sourceId,
                                 Runtime::QueryTerminationType) override;

    bool
    canTriggerEndOfStream(QueryId queryId, QuerySubPlanId subPlanId, OperatorId sourceId, Runtime::QueryTerminationType) override;

    bool notifyQueryStatusChange(QueryId queryId,
                                 QuerySubPlanId subQueryId,
                                 Runtime::Execution::ExecutableQueryPlanStatus newStatus) override;

    /**
     * @brief Method to let the Coordinator know of errors and exceptions
     * @param workerId of the worker that handled the failed query
     * @param errorMsg to describe the reason of the failure
     * @return true if Notification was successful, false otherwise
     */
    bool notifyErrors(uint64_t workerId, std::string errorMsg);

    /**
     * @brief Method to get worker id
     * @return worker id
    */
    uint64_t getWorkerId();

    /**
     * @brief Method to get numberOfBuffersPerEpoch
     * @return numberOfBuffersPerEpoch
    */
    uint64_t getNumberOfBuffersPerEpoch();

    const Configurations::WorkerConfigurationPtr& getWorkerConfiguration() const;

    /**
      * @brief method to propagate new epoch timestamp to coordinator
      * @param timestamp: max timestamp of current epoch
      * @param queryId: identifies what query sends punctuation
      * @return bool indicating success
      */
    bool notifyEpochTermination(uint64_t timestamp, uint64_t querySubPlanId) override;

    /**
      * @brief method that enables the worker to send errors to the Coordinator. Calls the notifyError method
      * @param signalNumber
      * @param string with exception
      * @return bool indicating success
      */
    void onFatalError(int signalNumber, std::string string) override;

    /**
      * @brief method that enables the worker to send exceptions to the Coordinator. Calls the notifyError method
      * @param ptr exception pointer
      * @param string with exception
      * @return bool indicating success
      */
    void onFatalException(std::shared_ptr<std::exception> ptr, std::string string) override;

    /**
     * get the class containing all location info on this worker if it is a field node and the functionality to
     * query the current position if it is a field node
     * @return
     */
    x::Spatial::Mobility::Experimental::LocationProviderPtr getLocationProvider();

    /**
     * get the class taking care of the trajectory prediction of mobile devices. Will return nullptr if the node is not mobile
     * @return
     */
    x::Spatial::Mobility::Experimental::ReconnectSchedulePredictorPtr getTrajectoryPredictor();

    x::Spatial::Mobility::Experimental::WorkerMobilityHandlerPtr getMobilityHandler();

  private:
    /**
     * @brief method to register physical source with the coordinator
     * @param physicalSources: physical sources containing relevant information
     * @return bool indicating success
     */
    bool registerPhysicalSources(const std::vector<PhysicalSourcePtr>& physicalSources);

    /**
     * @brief this method will start the GRPC Worker server which is responsible for reacting to calls
     */
    void buildAndStartGRPCServer(const std::shared_ptr<std::promise<int>>& prom);

    /**
     * @brief helper method to ensure client is connected before calling rpc functions
     * @return
     */
    bool waitForConnect() const;

    void handleRpcs(WorkerRPCServer& service);

    const Configurations::WorkerConfigurationPtr workerConfig;
    std::atomic<uint16_t> localWorkerRpcPort;
    std::string rpcAddress;
    x::Spatial::Mobility::Experimental::LocationProviderPtr locationProvider;
    x::Spatial::Mobility::Experimental::ReconnectSchedulePredictorPtr trajectoryPredictor;
    x::Spatial::Mobility::Experimental::WorkerMobilityHandlerPtr workerMobilityHandler;
    std::atomic<bool> isRunning{false};
    TopologyNodeId workerId;
    HealthCheckServicePtr healthCheckService;

    std::unique_ptr<grpc::Server> rpcServer;
    std::shared_ptr<std::thread> rpcThread;
    std::shared_ptr<std::thread> statisticOutputThread;
    std::unique_ptr<grpc::ServerCompletionQueue> completionQueue;
    Runtime::NodeEnginePtr nodeEngine;
    Monitoring::MonitoringAgentPtr monitoringAgent;
    Monitoring::MetricStorePtr metricStore;
    CoordinatorRPCClientPtr coordinatorRpcClient;
    std::atomic<bool> connected{false};
    uint32_t parentId;
    x::Configurations::Spatial::Mobility::Experimental::WorkerMobilityConfigurationPtr mobilityConfig;
};
using xWorkerPtr = std::shared_ptr<xWorker>;

}// namespace x
#endif// x_CORE_INCLUDE_COMPONENTS_xWORKER_HPP_
