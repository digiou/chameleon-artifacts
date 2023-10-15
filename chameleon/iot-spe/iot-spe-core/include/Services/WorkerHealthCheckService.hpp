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

#ifndef x_CORE_INCLUDE_SERVICES_WORKERHEALTHCHECKSERVICE_HPP_
#define x_CORE_INCLUDE_SERVICES_WORKERHEALTHCHECKSERVICE_HPP_

#include <Services/AbstractHealthCheckService.hpp>

namespace x {
class xWorker;
using xWorkerPtr = std::shared_ptr<xWorker>;

/**
 * @brief: This class is responsible for handling requests related to monitor the alive status of nodes from the worker
 */
class WorkerHealthCheckService : public x::AbstractHealthCheckService {
  public:
    WorkerHealthCheckService(CoordinatorRPCClientPtr coordinatorRpcClient, std::string healthServiceName, xWorkerPtr worker);

    /**
     * Method to start the health checking
     */
    void startHealthCheck() override;

  private:
    CoordinatorRPCClientPtr coordinatorRpcClient;
    xWorkerPtr worker;
};

using WorkerHealthCheckServicePtr = std::shared_ptr<WorkerHealthCheckService>;

}// namespace x

#endif// x_CORE_INCLUDE_SERVICES_WORKERHEALTHCHECKSERVICE_HPP_
