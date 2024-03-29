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

#ifndef x_CORE_INCLUDE_SERVICES_COORDINATORHEALTHCHECKSERVICE_HPP_
#define x_CORE_INCLUDE_SERVICES_COORDINATORHEALTHCHECKSERVICE_HPP_

#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Services/AbstractHealthCheckService.hpp>
#include <Util/libcuckoo/cuckoohash_map.hh>
#include <stdint.h>

namespace x {

/**
 * @brief: This class is responsible for handling requests related to monitor the alive status of nodes from the coordinator
 */
class CoordinatorHealthCheckService : public x::AbstractHealthCheckService {
  public:
    CoordinatorHealthCheckService(TopologyManagerServicePtr topologyManagerService,
                                  std::string healthServiceName,
                                  Configurations::CoordinatorConfigurationPtr coordinatorConfiguration);

    /**
     * Method to start the health checking
     */
    void startHealthCheck() override;

  private:
    TopologyManagerServicePtr topologyManagerService;
    WorkerRPCClientPtr workerRPCClient;
    Configurations::CoordinatorConfigurationPtr coordinatorConfiguration;
};

using CoordinatorHealthCheckServicePtr = std::shared_ptr<CoordinatorHealthCheckService>;

}// namespace x

#endif// x_CORE_INCLUDE_SERVICES_COORDINATORHEALTHCHECKSERVICE_HPP_
