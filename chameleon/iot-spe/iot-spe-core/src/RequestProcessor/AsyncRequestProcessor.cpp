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

#include <Common/Identifiers.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <RequestProcessor/AsyncRequestProcessor.hpp>
#include <RequestProcessor/StorageHandles/SerialStorageHandler.hpp>
#include <RequestProcessor/StorageHandles/StorageDataStructures.hpp>
#include <RequestProcessor/StorageHandles/StorageHandlerType.hpp>
#include <RequestProcessor/StorageHandles/TwoPhaseLockingStorageHandler.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/ThreadNaming.hpp>
#include <deque>
#include <numeric>

namespace x::RequestProcessor::Experimental {

AsyncRequestProcessor::AsyncRequestProcessor(const StorageDataStructures& storageDataStructures)
    : running(true), nextFreeRequestId(1) {

    numOfThreads = storageDataStructures.coordinatorConfiguration->requestExecutorThreads.getValue();
    auto storageHandlerType = storageDataStructures.coordinatorConfiguration->storageHandlerType.getValue();
    switch (storageHandlerType) {
        case StorageHandlerType::SerialHandler: storageHandler = SerialStorageHandler::create(storageDataStructures); break;
        case StorageHandlerType::TwoPhaseLocking:
            storageHandler = TwoPhaseLockingStorageHandler::create(storageDataStructures);
            break;
        default: x_THROW_RUNTIME_ERROR("Unknown storage type supplied. Failed to initialize request executor.");
    }

    for (uint32_t i = 0; i < numOfThreads; ++i) {
        std::promise<bool> promise;
        completionFutures.emplace_back(promise.get_future());
        auto thread = std::thread([this, i, promise = std::move(promise)]() mutable {
            try {
                setThreadName("RequestExecThr-%d", i);
                runningRoutine();
                promise.set_value(true);
            } catch (const std::exception& ex) {
                promise.set_exception(std::make_exception_ptr(ex));
            }
        });
        thread.detach();
    }
}

RequestId AsyncRequestProcessor::runAsync(AbstractRequestPtr request) {
    if (!running) {
        x_WARNING("Cannot execute request, Async request executor is not running");
        return INVALID_REQUEST_ID;
    }
    std::unique_lock lock(workMutex);
    auto requestId = nextFreeRequestId;
    request->setId(requestId);
    nextFreeRequestId = (nextFreeRequestId % MAX_REQUEST_ID) + 1;
    asyncRequestQueue.emplace_back(std::move(request));
    cv.notify_all();
    return requestId;
}

bool AsyncRequestProcessor::stop() {
    bool expected = true;
    if (running.compare_exchange_strong(expected, false)) {
        try {
            {
                std::unique_lock lock(workMutex);
                for (uint32_t i = 0; i < numOfThreads; ++i) {
                    asyncRequestQueue.emplace_back(std::make_shared<FlushRequest>());
                }
                cv.notify_all();
            }
            auto result = std::accumulate(completionFutures.begin(), completionFutures.end(), true, [](bool acc, auto& future) {
                return acc && future.get();
            });
            x_ASSERT(result, "Cannot shut down AsyncRequestExecutor");
            {
                std::unique_lock lock(workMutex);
                asyncRequestQueue.clear();
            }
            return result;
        } catch (std::exception const& ex) {
            x_ASSERT(false, "Cannot shut down AsyncRequestExecutor");
        }
    }
    return false;
}

void AsyncRequestProcessor::runningRoutine() {
    while (true) {
        std::unique_lock lock(workMutex);
        while (asyncRequestQueue.empty()) {
            cv.wait(lock);
        }
        if (running) {
            AbstractRequestPtr abstractRequest = asyncRequestQueue.front();
            asyncRequestQueue.pop_front();
            lock.unlock();

            //execute request logic
            std::vector<AbstractRequestPtr> nextRequests = abstractRequest->execute(storageHandler);

            //queue follow up requests
            for (auto followUpRequest : nextRequests) {
                runAsync(std::move(followUpRequest));
            }
        } else {
            break;
        }
    }
}

AsyncRequestProcessor::~AsyncRequestProcessor() { stop(); }

}// namespace x::RequestProcessor::Experimental
