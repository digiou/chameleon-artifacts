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

#ifndef x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_COORDINATORCONFIGURATION_HPP_
#define x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_COORDINATORCONFIGURATION_HPP_

#include <Configurations/BaseConfiguration.hpp>
#include <Configurations/Coordinator/ElegantConfigurations.hpp>
#include <Configurations/Coordinator/LogicalSourceFactory.hpp>
#include <Configurations/Coordinator/OptimizerConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <REST/ServerTypes.hpp>
#include <RequestProcessor/StorageHandles/StorageHandlerType.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>

namespace x {

class LogicalSource;
using LogicalSourcePtr = std::shared_ptr<LogicalSource>;

namespace Configurations {

class CoordinatorConfiguration;
using CoordinatorConfigurationPtr = std::shared_ptr<CoordinatorConfiguration>;

/**
 * @brief Configuration options for the Coordinator.
 */
class CoordinatorConfiguration : public BaseConfiguration {
  public:
    /**
     * @brief IP of the REST server.
     */
    StringOption restIp = {REST_IP_CONFIG, "127.0.0.1", "x ip of the REST server."};

    /**
     * @brief Port of the REST server.
     */
    UIntOption restPort = {REST_PORT_CONFIG, 8081, "Port exposed for rest endpoints"};

    /**
     * @brief IP of the Coordinator.
     */
    StringOption coordinatorIp = {COORDINATOR_IP_CONFIG, "127.0.0.1", "RPC IP address of x Coordinator."};

    /**
     * @brief Port for the RPC server of the Coordinator.
     * This is used to receive control messages.
     */
    UIntOption rpcPort = {RPC_PORT_CONFIG, 4000, "RPC server port of the Coordinator"};

    /**
     * @brief Port of the Data server of the Coordinator.
     * This is used to receive data at the coordinator.
     */
    UIntOption dataPort = {DATA_PORT_CONFIG, 0, "Data server port of the Coordinator"};

    /**
     * @brief The current log level. Controls the detail of log messages.
     */
    EnumOption<LogLevel> logLevel = {LOG_LEVEL_CONFIG,
                                     LogLevel::LOG_INFO,
                                     "The log level (LOG_NONE, LOG_WARNING, LOG_DEBUG, LOG_INFO, LOG_TRACE)"};

    /**
     * @brief Indicates if the monitoring stack is enables.
     */
    BoolOption enableMonitoring = {ENABLE_MONITORING_CONFIG, false, "Enable monitoring"};

    /**
     * @brief Indicates if new request execution module is to be used
     */
    BoolOption enableNewRequestExecutor = {ENABLE_NEW_REQUEST_EXECUTOR_CONFIG, false, "Enable New Request Executor"};

    /**
     * @brief Indicates the number of request executor threads
     */
    UIntOption requestExecutorThreads = {REQUEST_EXECUTOR_THREAD_CONFIG, 1, "Number of request executor thread"};

    /**
     * @brief Storage handler for request executor
     */
    EnumOption<RequestProcessor::Experimental::StorageHandlerType> storageHandlerType = {
        STORAGE_HANDLER_TYPE_CONFIG,
        RequestProcessor::Experimental::StorageHandlerType::TwoPhaseLocking,
        "The Storage Handler Type (TwoPhaseLocking, SerialHandler)"};

    /**
     * @brief Enable reconfiguration of running query plans.
     */
    BoolOption enableQueryReconfiguration = {ENABLE_QUERY_RECONFIGURATION,
                                             false,
                                             "Enable reconfiguration of running query plans. (Default: false)"};

    /**
     * @brief Configures different properties for the query optimizer.
     */
    OptimizerConfiguration optimizer = {OPTIMIZER_CONFIG, "Defix the configuration for the optimizer."};

    /**
     * @brief Allows the configuration of logical sources at the coordinator.
     * @deprecated This is currently only used for testing and will be removed.
     */
    SequenceOption<WrapOption<LogicalSourcePtr, LogicalSourceFactory>> logicalSources = {LOGICAL_SOURCES, "Logical Sources"};

    /**
     * @brief Configuration yaml path.
     * @warning this is just a placeholder configuration
     */
    StringOption configPath = {CONFIG_PATH, "", "Path to configuration file."};

    /**
     * @brief Configures different properties of the internal worker in the coordinator configuration file and on the command line.
     */
    WorkerConfiguration worker = {WORKER_CONFIG, "Defix the configuration for the worker."};

    /**
     * @brief Path to a dedicated configuration file for the internal worker.
     */
    StringOption workerConfigPath = {WORKER_CONFIG_PATH, "", "Path to a configuration file for the internal worker."};

    /**
     * @brief Configuration of waiting time of the coordinator health check.
     * Set the number of seconds waiting to perform health checks
     */
    UIntOption coordinatorHealthCheckWaitTime = {HEALTH_CHECK_WAIT_TIME, 1, "Number of seconds to wait between health checks"};

    /**
     * @brief The allowed origin for CORS requests which will be sent as part of the header of the http responses of the rest server
     */
    StringOption restServerCorsAllowedOrigin = {REST_SERVER_CORS_ORIGIN,
                                                "",
                                                "The allowed origins to be set in the header of the responses to rest requests"};

    /**
     * @brief ELEGANT related configuration parameters
     */
    ElegantConfigurations elegantConfiguration = {ELEGANT, "Define ELEGANT configuration"};

    /**
     * @brief Create a default CoordinatorConfiguration object with default values.
     * @return A CoordinatorConfiguration object with default values.
     */
    static std::shared_ptr<CoordinatorConfiguration> createDefault() { return std::make_shared<CoordinatorConfiguration>(); }

    /**
     * Create a CoordinatorConfiguration object and set values from the POSIX command line parameters stored in argv.
     * @param argc The argc parameter given to the main function.
     * @param argv The argv parameter given to the main function.
     * @return A configured configuration object.
     */
    static CoordinatorConfigurationPtr create(const int argc, const char** argv);

  private:
    std::vector<Configurations::BaseOption*> getOptions() override {
        return {&restIp,
                &coordinatorIp,
                &rpcPort,
                &restPort,
                &dataPort,
                &logLevel,
                &enableQueryReconfiguration,
                &enableMonitoring,
                &configPath,
                &worker,
                &workerConfigPath,
                &optimizer,
                &logicalSources,
                &coordinatorHealthCheckWaitTime,
                &restServerCorsAllowedOrigin};
    }
};

}// namespace Configurations
}// namespace x

#endif// x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_COORDINATORCONFIGURATION_HPP_
