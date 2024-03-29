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

#include <API/Schema.hpp>
#include <Catalogs/Source/LogicalSource.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <CoordinatorRPCService.pb.h>
#include <Services/SourceCatalogService.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>

namespace x {

SourceCatalogService::SourceCatalogService(Catalogs::Source::SourceCatalogPtr sourceCatalog)
    : sourceCatalog(std::move(sourceCatalog)) {
    x_DEBUG("SourceCatalogService()");
    x_ASSERT(this->sourceCatalog, "sourceCatalogPtr has to be valid");
}

bool SourceCatalogService::registerPhysicalSource(TopologyNodePtr topologyNode,
                                                  const std::string& physicalSourceName,
                                                  const std::string& logicalSourceName) {
    if (!topologyNode) {
        x_ERROR("SourceCatalogService::RegisterPhysicalSource node not found");
        return false;
    }

    if (!sourceCatalog->containsLogicalSource(logicalSourceName)) {
        x_ERROR("SourceCatalogService::RegisterPhysicalSource logical source does not exist {}", logicalSourceName);
        return false;
    }

    x_DEBUG("SourceCatalogService::RegisterPhysicalSource: try to register physical node id {} physical source= {} logical "
              "source= {}",
              topologyNode->getId(),
              physicalSourceName,
              logicalSourceName);
    std::unique_lock<std::mutex> lock(addRemovePhysicalSource);
    auto physicalSource = PhysicalSource::create(logicalSourceName, physicalSourceName);
    auto logicalSource = sourceCatalog->getLogicalSource(logicalSourceName);
    Catalogs::Source::SourceCatalogEntryPtr sce =
        std::make_shared<Catalogs::Source::SourceCatalogEntry>(physicalSource, logicalSource, topologyNode);
    bool success = sourceCatalog->addPhysicalSource(logicalSourceName, sce);
    if (!success) {
        x_ERROR("SourceCatalogService::RegisterPhysicalSource: adding physical source was not successful.");
        return false;
    }
    return success;
}

bool SourceCatalogService::unregisterPhysicalSource(TopologyNodePtr topologyNode,
                                                    const std::string& physicalSourceName,
                                                    const std::string& logicalSourceName) {
    x_DEBUG(
        "SourceCatalogService::UnregisterPhysicalSource: try to remove physical source with name {} logical name {} workerId= {}",
        physicalSourceName,
        logicalSourceName,
        topologyNode->getId());
    std::unique_lock<std::mutex> lock(addRemovePhysicalSource);

    if (!topologyNode) {
        x_DEBUG("SourceCatalogService::UnregisterPhysicalSource: sensor not found with workerId {}", topologyNode->getId());
        return false;
    }
    x_DEBUG("SourceCatalogService: node= {}", topologyNode->toString());

    bool success = sourceCatalog->removePhysicalSource(logicalSourceName, physicalSourceName, topologyNode->getId());
    if (!success) {
        x_ERROR("SourceCatalogService::RegisterPhysicalSource: removing physical source was not successful.");
        return false;
    }
    return success;
}

bool SourceCatalogService::registerLogicalSource(const std::string& logicalSourceName, const std::string& schemaString) {
    x_DEBUG("SourceCatalogService::registerLogicalSource: register logical source={} schema= {}",
              logicalSourceName,
              schemaString);
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->addLogicalSource(logicalSourceName, schemaString);
}

bool SourceCatalogService::registerLogicalSource(const std::string& logicalSourceName, SchemaPtr schema) {
    x_DEBUG("SourceCatalogService::registerLogicalSource: register logical source= {} schema= {}",
              logicalSourceName,
              schema->toString());
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->addLogicalSource(logicalSourceName, std::move(schema));
}

bool SourceCatalogService::updateLogicalSource(const std::string& logicalSourceName, SchemaPtr schema) {
    x_DEBUG("SourceCatalogService::update logical source {} with schema {}", logicalSourceName, schema->toString());
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->updateLogicalSource(logicalSourceName, std::move(schema));
}

bool SourceCatalogService::updateLogicalSource(const std::string& logicalSourceName, const std::string& schema) {
    x_DEBUG("SourceCatalogService::update logical source {} with schema {}", logicalSourceName, schema);
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->updateLogicalSource(logicalSourceName, schema);
}

bool SourceCatalogService::unregisterLogicalSource(const std::string& logicalSourceName) {
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    x_DEBUG("SourceCatalogService::unregisterLogicalSource: register logical source= {}", logicalSourceName);
    bool success = sourceCatalog->removeLogicalSource(logicalSourceName);
    return success;
}

LogicalSourcePtr SourceCatalogService::getLogicalSource(const std::string& logicalSourceName) {
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    auto logicalSource = sourceCatalog->getLogicalSource(logicalSourceName);
    return std::make_shared<LogicalSource>(*logicalSource);
}

std::map<std::string, std::string> SourceCatalogService::getAllLogicalSourceAsString() {
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->getAllLogicalSourceAsString();
}

std::vector<Catalogs::Source::SourceCatalogEntryPtr>
SourceCatalogService::getPhysicalSources(const std::string& logicalSourceName) {
    std::unique_lock<std::mutex> lock(addRemoveLogicalSource);
    return sourceCatalog->getPhysicalSources(logicalSourceName);
}

}//namespace x
