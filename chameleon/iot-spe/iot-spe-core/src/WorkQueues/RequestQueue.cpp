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
#include <Util/Logger/Logger.hpp>
#include <WorkQueues/RequestQueue.hpp>
#include <WorkQueues/RequestTypes/Request.hpp>
#include <algorithm>

namespace x {

RequestQueue::RequestQueue(uint64_t batchSize) : newRequestAvailable(false), batchSize(batchSize) {}

bool RequestQueue::add(const xRequestPtr& request) {
    std::unique_lock<std::mutex> lock(requestMutex);
    x_INFO("QueryRequestQueue: Adding a new query request : {}", request->toString());
    //TODO: identify and handle if more than one request for same query exists in the queue
    requestQueue.emplace_back(request);
    x_INFO("QueryCatalog: Marking that new request is available to be scheduled");
    setNewRequestAvailable(true);
    availabilityTrigger.notify_one();
    return true;
}

std::vector<xRequestPtr> RequestQueue::getNextBatch() {
    std::unique_lock<std::mutex> lock(requestMutex);
    //We are using conditional variable to prevent Lost Wakeup and Spurious Wakeup
    //ref: https://www.moderxcpp.com/index.php/c-core-guidelix-be-aware-of-the-traps-of-condition-variables
    availabilityTrigger.wait(lock, [&] {
        return isNewRequestAvailable();
    });
    x_INFO("QueryRequestQueue: Fetching Queries to Schedule");
    std::vector<xRequestPtr> queriesToSchedule;
    queriesToSchedule.reserve(batchSize);
    if (!requestQueue.empty()) {
        uint64_t currentBatchSize = 1;// todo why is this 1, not 0?
        //Prepare a batch of queryIdAndCatalogEntryMapping to schedule
        while (currentBatchSize <= batchSize && !requestQueue.empty()) {
            queriesToSchedule.emplace_back(requestQueue.front());
            requestQueue.pop_front();
            currentBatchSize++;
        }
        x_INFO("QueryRequestQueue: Optimizing {} queryIdAndCatalogEntryMapping.", queriesToSchedule.size());
        setNewRequestAvailable(!requestQueue.empty());
        return queriesToSchedule;
    }
    x_INFO("QueryRequestQueue: Nothing to schedule.");
    setNewRequestAvailable(!requestQueue.empty());
    return queriesToSchedule;
}

void RequestQueue::insertPoisonPill() {
    std::unique_lock<std::mutex> lock(requestMutex);
    x_INFO("QueryRequestQueue: Shutdown is called. Inserting Poison pill in the query request queue.");
    setNewRequestAvailable(true);
    availabilityTrigger.notify_one();
}

bool RequestQueue::isNewRequestAvailable() const { return newRequestAvailable; }

void RequestQueue::setNewRequestAvailable(bool newRequestAvailable) { this->newRequestAvailable = newRequestAvailable; }

}// namespace x