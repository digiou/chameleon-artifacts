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

#include <Components/xWorker.hpp>
#include <GRPC/CoordinatorRPCClient.hpp>
#include <Services/WorkerHealthCheckService.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadNaming.hpp>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

namespace x {

WorkerHealthCheckService::WorkerHealthCheckService(CoordinatorRPCClientPtr coordinatorRpcClient,
                                                   std::string healthServiceName,
                                                   xWorkerPtr worker)
    : coordinatorRpcClient(coordinatorRpcClient), worker(worker) {
    id = coordinatorRpcClient->getId();
    this->healthServiceName = healthServiceName;
}

void WorkerHealthCheckService::startHealthCheck() {
    x_DEBUG("WorkerHealthCheckService::startHealthCheck worker id= {}", id);
    isRunning = true;
    x_DEBUG("start health checking on worker");
    healthCheckingThread = std::make_shared<std::thread>(([this]() {
        setThreadName("xHealth");
        x_TRACE("xWorker: start health checking");
        auto waitTime = std::chrono::seconds(worker->getWorkerConfiguration()->workerHealthCheckWaitTime.getValue());
        while (isRunning) {
            x_TRACE("xWorker::healthCheck for worker id=  {}", coordinatorRpcClient->getId());

            bool isAlive = coordinatorRpcClient->checkCoordinatorHealth(healthServiceName);
            if (isAlive) {
                x_TRACE("xWorker::healthCheck: for worker id={} is alive", coordinatorRpcClient->getId());
            } else {
                x_ERROR("xWorker::healthCheck: for worker id={} coordinator went down so shutting down the worker with ip",
                          coordinatorRpcClient->getId());
                worker->stop(true);
            }
            {
                std::unique_lock<std::mutex> lk(cvMutex);
                cv.wait_for(lk, waitTime, [this] {
                    return isRunning == false;
                });
            }
        }
        //        we have to wait until the code above terminates to proceed afterwards with shutdown of the rpc server (can be delayed due to sleep)
        shutdownRPC->set_value(true);
        x_DEBUG("xWorker::healthCheck: stop health checking id= {}", id);
    }));
}

}// namespace x