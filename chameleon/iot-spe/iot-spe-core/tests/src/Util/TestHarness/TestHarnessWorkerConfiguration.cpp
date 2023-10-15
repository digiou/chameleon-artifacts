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
#include <Util/TestHarxs/TestHarxsWorkerConfiguration.hpp>

namespace x {

TestHarxsWorkerConfigurationPtr TestHarxsWorkerConfiguration::create(WorkerConfigurationPtr workerConfiguration,
                                                                         uint32_t workerId) {
    return std::make_shared<TestHarxsWorkerConfiguration>(
        TestHarxsWorkerConfiguration(std::move(workerConfiguration), TestHarxsWorkerSourceType::NonSource, workerId));
}

TestHarxsWorkerConfigurationPtr TestHarxsWorkerConfiguration::create(WorkerConfigurationPtr workerConfiguration,
                                                                         std::string logicalSourceName,
                                                                         std::string physicalSourceName,
                                                                         TestHarxsWorkerSourceType sourceType,
                                                                         uint32_t workerId) {
    return std::make_shared<TestHarxsWorkerConfiguration>(TestHarxsWorkerConfiguration(std::move(workerConfiguration),
                                                                                           std::move(logicalSourceName),
                                                                                           std::move(physicalSourceName),
                                                                                           sourceType,
                                                                                           workerId));
}

TestHarxsWorkerConfiguration::TestHarxsWorkerConfiguration(WorkerConfigurationPtr workerConfiguration,
                                                               TestHarxsWorkerSourceType sourceType,
                                                               uint32_t workerId)
    : workerConfiguration(std::move(workerConfiguration)), sourceType(sourceType), workerId(workerId){};

TestHarxsWorkerConfiguration::TestHarxsWorkerConfiguration(WorkerConfigurationPtr workerConfiguration,
                                                               std::string logicalSourceName,
                                                               std::string physicalSourceName,
                                                               TestHarxsWorkerSourceType sourceType,
                                                               uint32_t workerId)
    : workerConfiguration(std::move(workerConfiguration)), logicalSourceName(std::move(logicalSourceName)),
      physicalSourceName(std::move(physicalSourceName)), sourceType(sourceType), workerId(workerId){};

void TestHarxsWorkerConfiguration::setQueryStatusListener(const xWorkerPtr& xWorker) { this->xWorker = xWorker; }

PhysicalSourceTypePtr TestHarxsWorkerConfiguration::getPhysicalSourceType() const { return physicalSource; }

const WorkerConfigurationPtr& TestHarxsWorkerConfiguration::getWorkerConfiguration() const { return workerConfiguration; }

TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType TestHarxsWorkerConfiguration::getSourceType() const {
    return sourceType;
}
void TestHarxsWorkerConfiguration::setPhysicalSourceType(PhysicalSourceTypePtr physicalSource) {
    this->physicalSource = physicalSource;
}
const std::vector<uint8_t*>& TestHarxsWorkerConfiguration::getRecords() const { return records; }

void TestHarxsWorkerConfiguration::addRecord(uint8_t* record) { records.push_back(record); }

uint32_t TestHarxsWorkerConfiguration::getWorkerId() const { return workerId; }

const std::string& TestHarxsWorkerConfiguration::getLogicalSourceName() const { return logicalSourceName; }

const std::string& TestHarxsWorkerConfiguration::getPhysicalSourceName() const { return physicalSourceName; }
const xWorkerPtr& TestHarxsWorkerConfiguration::getxWorker() const { return xWorker; }
}// namespace x