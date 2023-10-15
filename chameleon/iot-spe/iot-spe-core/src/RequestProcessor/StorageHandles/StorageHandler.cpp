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

#include <RequestProcessor/StorageHandles/StorageHandler.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::RequestProcessor::Experimental {

void StorageHandler::acquireResources(const RequestId, const std::vector<ResourceType>&) {}

void StorageHandler::releaseResources(const RequestId) {}

GlobalExecutionPlanHandle StorageHandler::getGlobalExecutionPlanHandle(RequestId) { x_NOT_IMPLEMENTED(); }

TopologyHandle StorageHandler::getTopologyHandle(RequestId) { x_NOT_IMPLEMENTED(); }

QueryCatalogServiceHandle StorageHandler::getQueryCatalogServiceHandle(RequestId) { x_NOT_IMPLEMENTED(); }

GlobalQueryPlanHandle StorageHandler::getGlobalQueryPlanHandle(RequestId) { x_NOT_IMPLEMENTED(); }

SourceCatalogHandle StorageHandler::getSourceCatalogHandle(RequestId) { x_NOT_IMPLEMENTED(); }

UDFCatalogHandle StorageHandler::getUDFCatalogHandle(RequestId) { x_NOT_IMPLEMENTED(); }

CoordinatorConfigurationHandle StorageHandler::getCoordinatorConfiguration(RequestId) { x_NOT_IMPLEMENTED(); }

}// namespace x::RequestProcessor::Experimental