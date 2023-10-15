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

#ifndef x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxSWORKERCONFIGURATION_HPP_
#define x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxSWORKERCONFIGURATION_HPP_

#include <API/Schema.hpp>
#include <Catalogs/Source/LogicalSource.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Components/xCoordinator.hpp>
#include <Components/xWorker.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Nodes/Node.hpp>
#include <utility>

namespace x {

class TestHarxsWorkerConfiguration;
using TestHarxsWorkerConfigurationPtr = std::shared_ptr<TestHarxsWorkerConfiguration>;

/**
 * @brief A class to keep Configurations of different nodes in the topology
 */
class TestHarxsWorkerConfiguration {

  public:
    enum class TestHarxsWorkerSourceType : int8_t { CSVSource, MemorySource, LambdaSource, NonSource };

    static TestHarxsWorkerConfigurationPtr create(WorkerConfigurationPtr workerConfiguration, uint32_t workerId);

    static TestHarxsWorkerConfigurationPtr create(WorkerConfigurationPtr workerConfiguration,
                                                    std::string logicalSourceName,
                                                    std::string physicalSourceName,
                                                    TestHarxsWorkerSourceType sourceType,
                                                    uint32_t workerId);

    void setQueryStatusListener(const xWorkerPtr& xWorker);

    const WorkerConfigurationPtr& getWorkerConfiguration() const;
    TestHarxsWorkerSourceType getSourceType() const;
    PhysicalSourceTypePtr getPhysicalSourceType() const;
    void setPhysicalSourceType(PhysicalSourceTypePtr physicalSource);
    const std::vector<uint8_t*>& getRecords() const;
    void addRecord(uint8_t* record);
    uint32_t getWorkerId() const;
    const std::string& getLogicalSourceName() const;
    const std::string& getPhysicalSourceName() const;
    const xWorkerPtr& getxWorker() const;

  private:
    TestHarxsWorkerConfiguration(WorkerConfigurationPtr workerConfiguration,
                                   std::string logicalSourceName,
                                   std::string physicalSourceName,
                                   TestHarxsWorkerSourceType sourceType,
                                   uint32_t workerId);
    TestHarxsWorkerConfiguration(WorkerConfigurationPtr workerConfiguration,
                                   TestHarxsWorkerSourceType sourceType,
                                   uint32_t workerId);
    WorkerConfigurationPtr workerConfiguration;
    std::string logicalSourceName;
    std::string physicalSourceName;
    TestHarxsWorkerSourceType sourceType;
    std::vector<uint8_t*> records;
    uint32_t workerId;
    xWorkerPtr xWorker;
    PhysicalSourceTypePtr physicalSource;
};

}// namespace x

#endif// x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxSWORKERCONFIGURATION_HPP_
