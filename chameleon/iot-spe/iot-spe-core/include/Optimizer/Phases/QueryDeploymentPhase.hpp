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

#ifndef x_CORE_INCLUDE_PHASES_QUERYDEPLOYMENTPHASE_HPP_
#define x_CORE_INCLUDE_PHASES_QUERYDEPLOYMENTPHASE_HPP_

#include <Common/Identifiers.hpp>
#include <iostream>
#include <memory>
#include <vector>

namespace x {

namespace Configurations {
class CoordinatorConfiguration;
using CoordinatorConfigurationPtr = std::shared_ptr<CoordinatorConfiguration>;
}// namespace Configurations

class WorkerRPCClient;
using WorkerRPCClientPtr = std::shared_ptr<WorkerRPCClient>;

class ExecutionNode;
using ExecutionNodePtr = std::shared_ptr<ExecutionNode>;

class GlobalExecutionPlan;
using GlobalExecutionPlanPtr = std::shared_ptr<GlobalExecutionPlan>;

class QueryDeploymentPhase;
using QueryDeploymentPhasePtr = std::shared_ptr<QueryDeploymentPhase>;

class QueryCatalogService;
using QueryCatalogServicePtr = std::shared_ptr<QueryCatalogService>;

class SharedQueryPlan;
using SharedQueryPlanPtr = std::shared_ptr<SharedQueryPlan>;

/**
 * @brief The query deployment phase is responsible for deploying the query plan for a query to respective worker nodes.
 */
class QueryDeploymentPhase {
  public:
    /**
     * @brief Returns a smart pointer to the QueryDeploymentPhase
     * @param globalExecutionPlan : global execution plan
     * @param queryCatalogService: query catalog service
     * @param coordinatorConfiguration: coordinator configuration
     * @return shared pointer to the instance of QueryDeploymentPhase
     */
    static QueryDeploymentPhasePtr create(const GlobalExecutionPlanPtr& globalExecutionPlan,
                                          const QueryCatalogServicePtr& queryCatalogService,
                                          const Configurations::CoordinatorConfigurationPtr& coordinatorConfiguration);

    /**
     * @brief method for deploying and starting the query
     * @param queryId : the query Id of the query to be deployed and started
     * @throws ExecutionNodeNotFoundException: Unable to find ExecutionNodes where the query {sharedQueryId} is deployed
     */
    void execute(const SharedQueryPlanPtr& sharedQueryPlan);

  private:
    explicit QueryDeploymentPhase(const GlobalExecutionPlanPtr& globalExecutionPlan,
                                  const QueryCatalogServicePtr& queryCatalogService,
                                  bool accelerateJavaUDFs,
                                  const std::string& accelerationServiceURL);
    /**
     * @brief method send query to nodes
     * @param queryId
     * @return bool indicating success
     * todo: #3821 change to specific exception
     * @throws QueryDeploymentException The bytecode list of classes implementing the UDF must contain the fully-qualified name of the UDF
     * @throws QueryDeploymentException: Error in call to Elegant acceleration service with code
     * @throws QueryDeploymentException: QueryDeploymentPhase : unable to find query sub plan with id
     */
    void deployQuery(QueryId queryId, const std::vector<ExecutionNodePtr>& executionNodes);

    /**
     * @brief method to start a already deployed query
     * @param queryId
     */
    void startQuery(QueryId queryId, const std::vector<ExecutionNodePtr>& executionNodes);

    WorkerRPCClientPtr workerRPCClient;
    GlobalExecutionPlanPtr globalExecutionPlan;
    QueryCatalogServicePtr queryCatalogService;
    bool accelerateJavaUDFs;
    std::string accelerationServiceURL;

    const int32_t ELEGANT_SERVICE_TIMEOUT = 3000;

    //OpenCL payload constants
    const std::string DEVICE_INFO_KEY = "deviceInfo";
    const std::string DEVICE_INFO_NAME_KEY = "deviceName";
    const std::string DEVICE_INFO_DOUBLE_FP_SUPPORT_KEY = "doubleFPSupport";
    const std::string DEVICE_MAX_WORK_ITEMS_KEY = "maxWorkItems";
    const std::string DEVICE_MAX_WORK_ITEMS_DIM1_KEY = "dim1";
    const std::string DEVICE_MAX_WORK_ITEMS_DIM2_KEY = "dim2";
    const std::string DEVICE_MAX_WORK_ITEMS_DIM3_KEY = "dim3";
    const std::string DEVICE_INFO_ADDRESS_BITS_KEY = "deviceAddressBits";
    const std::string DEVICE_INFO_TYPE_KEY = "deviceType";
    const std::string DEVICE_INFO_EXTENSIONS_KEY = "deviceExtensions";
    const std::string DEVICE_INFO_AVAILABLE_PROCESSORS_KEY = "availableProcessors";
};
}// namespace x
#endif// x_CORE_INCLUDE_PHASES_QUERYDEPLOYMENTPHASE_HPP_
