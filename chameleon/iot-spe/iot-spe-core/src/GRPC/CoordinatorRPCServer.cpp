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

#include <Configurations/WorkerConfigurationKeys.hpp>
#include <Configurations/WorkerPropertyKeys.hpp>
#include <Exceptions/InvalidQueryStateException.hpp>
#include <GRPC/CoordinatorRPCServer.hpp>
#include <Monitoring/Metrics/Gauge/RegistrationMetrics.hpp>
#include <Monitoring/Metrics/Metric.hpp>
#include <Monitoring/MonitoringManager.hpp>
#include <Services/LocationService.hpp>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Services/ReplicationService.hpp>
#include <Services/TopologyManagerService.hpp>
#include <Spatial/Mobility/ReconnectSchedulePredictors/ReconnectPoint.hpp>
#include <Util/Experimental/SpatialTypeUtility.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>

using namespace x;

CoordinatorRPCServer::CoordinatorRPCServer(QueryServicePtr queryService,
                                           TopologyManagerServicePtr topologyManagerService,
                                           SourceCatalogServicePtr sourceCatalogService,
                                           QueryCatalogServicePtr queryCatalogService,
                                           Monitoring::MonitoringManagerPtr monitoringManager,
                                           ReplicationServicePtr replicationService,
                                           LocationServicePtr locationService)
    : queryService(std::move(queryService)), topologyManagerService(std::move(topologyManagerService)),
      sourceCatalogService(std::move(sourceCatalogService)), queryCatalogService(std::move(queryCatalogService)),
      monitoringManager(std::move(monitoringManager)), replicationService(std::move(replicationService)),
      locationService(std::move(locationService)){};

Status CoordinatorRPCServer::RegisterWorker(ServerContext*,
                                            const RegisterWorkerRequest* registrationRequest,
                                            RegisterWorkerReply* reply) {

    x_DEBUG("Received worker registration request {}", registrationRequest->DebugString());
    auto configWorkerId = registrationRequest->workerid();
    auto address = registrationRequest->address();
    auto grpcPort = registrationRequest->grpcport();
    auto dataPort = registrationRequest->dataport();
    auto slots = registrationRequest->numberofslots();
    //construct worker property from the request
    std::map<std::string, std::any> workerProperties;
    workerProperties[x::Worker::Properties::MAINTENANCE] =
        false;//During registration, we assume the node is not under maintenance
    workerProperties[x::Worker::Configuration::TENSORFLOW_SUPPORT] = registrationRequest->tfsupported();
    workerProperties[x::Worker::Configuration::JAVA_UDF_SUPPORT] = registrationRequest->javaudfsupported();
    workerProperties[x::Worker::Configuration::SPATIAL_SUPPORT] =
        x::Spatial::Util::SpatialTypeUtility::protobufEnumToNodeType(registrationRequest->spatialtype());

    x_DEBUG("TopologyManagerService::RegisterNode: request ={}", registrationRequest->DebugString());
    TopologyNodeId workerId =
        topologyManagerService->registerWorker(configWorkerId, address, grpcPort, dataPort, slots, workerProperties);

    x::Spatial::DataTypes::Experimental::GeoLocation geoLocation(registrationRequest->waypoint().geolocation().lat(),
                                                                   registrationRequest->waypoint().geolocation().lng());

    if (!topologyManagerService->addGeoLocation(workerId, std::move(geoLocation))) {
        x_ERROR("Unable to update geo location of the topology");
        reply->set_workerid(0);
        return Status::CANCELLED;
    }

    auto registrationMetrics =
        std::make_shared<Monitoring::Metric>(Monitoring::RegistrationMetrics(registrationRequest->registrationmetrics()),
                                             Monitoring::MetricType::RegistrationMetric);
    registrationMetrics->getValue<Monitoring::RegistrationMetrics>().nodeId = workerId;
    monitoringManager->addMonitoringData(workerId, registrationMetrics);

    if (workerId != 0) {
        x_DEBUG("CoordinatorRPCServer::RegisterNode: success id={}", workerId);
        reply->set_workerid(workerId);
        return Status::OK;
    }
    x_DEBUG("CoordinatorRPCServer::RegisterNode: failed");
    reply->set_workerid(0);
    return Status::CANCELLED;
}

Status
CoordinatorRPCServer::UnregisterWorker(ServerContext*, const UnregisterWorkerRequest* request, UnregisterWorkerReply* reply) {
    x_DEBUG("CoordinatorRPCServer::UnregisterNode: request ={}", request->DebugString());

    auto spatialType = topologyManagerService->findNodeWithId(request->workerid())->getSpatialNodeType();
    bool success = topologyManagerService->removeGeoLocation(request->workerid())
        || spatialType == x::Spatial::Experimental::SpatialType::NO_LOCATION
        || spatialType == x::Spatial::Experimental::SpatialType::INVALID;
    if (success) {
        if (!topologyManagerService->unregisterNode(request->workerid())) {
            x_ERROR("CoordinatorRPCServer::UnregisterNode: Worker was not removed");
            reply->set_success(false);
            return Status::CANCELLED;
        }
        monitoringManager->removeMonitoringNode(request->workerid());
        x_DEBUG("CoordinatorRPCServer::UnregisterNode: Worker successfully removed");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::UnregisterNode: sensor was not removed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::RegisterPhysicalSource(ServerContext*,
                                                    const RegisterPhysicalSourcesRequest* request,
                                                    RegisterPhysicalSourcesReply* reply) {
    x_DEBUG("CoordinatorRPCServer::RegisterPhysicalSource: request ={}", request->DebugString());
    TopologyNodePtr physicalNode = this->topologyManagerService->findNodeWithId(request->workerid());
    for (const auto& physicalSourceDefinition : request->physicalsources()) {
        bool success = sourceCatalogService->registerPhysicalSource(physicalNode,
                                                                    physicalSourceDefinition.physicalsourcename(),
                                                                    physicalSourceDefinition.logicalsourcename());
        if (!success) {
            x_ERROR("CoordinatorRPCServer::RegisterPhysicalSource failed");
            reply->set_success(false);
            return Status::CANCELLED;
        }
    }
    x_DEBUG("CoordinatorRPCServer::RegisterPhysicalSource Succeed");
    reply->set_success(true);
    return Status::OK;
}

Status CoordinatorRPCServer::UnregisterPhysicalSource(ServerContext*,
                                                      const UnregisterPhysicalSourceRequest* request,
                                                      UnregisterPhysicalSourceReply* reply) {
    x_DEBUG("CoordinatorRPCServer::UnregisterPhysicalSource: request ={}", request->DebugString());

    TopologyNodePtr physicalNode = this->topologyManagerService->findNodeWithId(request->workerid());
    bool success =
        sourceCatalogService->unregisterPhysicalSource(physicalNode, request->physicalsourcename(), request->logicalsourcename());

    if (success) {
        x_DEBUG("CoordinatorRPCServer::UnregisterPhysicalSource success");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::UnregisterPhysicalSource failed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::RegisterLogicalSource(ServerContext*,
                                                   const RegisterLogicalSourceRequest* request,
                                                   RegisterLogicalSourceReply* reply) {
    x_DEBUG("CoordinatorRPCServer::RegisterLogicalSource: request = {}", request->DebugString());

    bool success = sourceCatalogService->registerLogicalSource(request->logicalsourcename(), request->sourceschema());

    if (success) {
        x_DEBUG("CoordinatorRPCServer::RegisterLogicalSource success");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::RegisterLogicalSource failed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::UnregisterLogicalSource(ServerContext*,
                                                     const UnregisterLogicalSourceRequest* request,
                                                     UnregisterLogicalSourceReply* reply) {
    x_DEBUG("CoordinatorRPCServer::RegisterLogicalSource: request ={}", request->DebugString());

    bool success = sourceCatalogService->unregisterLogicalSource(request->logicalsourcename());
    if (success) {
        x_DEBUG("CoordinatorRPCServer::UnregisterLogicalSource success");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::UnregisterLogicalSource failed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::AddParent(ServerContext*, const AddParentRequest* request, AddParentReply* reply) {
    x_DEBUG("CoordinatorRPCServer::AddParent: request = {}", request->DebugString());

    bool success = topologyManagerService->addParent(request->childid(), request->parentid());
    if (success) {
        x_DEBUG("CoordinatorRPCServer::AddParent success");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::AddParent failed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::ReplaceParent(ServerContext*, const ReplaceParentRequest* request, ReplaceParentReply* reply) {
    x_DEBUG("CoordinatorRPCServer::ReplaceParent: request = {}", request->DebugString());

    bool success = topologyManagerService->removeParent(request->childid(), request->oldparent());
    if (success) {
        x_DEBUG("CoordinatorRPCServer::ReplaceParent success removeParent");
        bool success2 = topologyManagerService->addParent(request->childid(), request->newparent());
        if (success2) {
            x_DEBUG("CoordinatorRPCServer::ReplaceParent success addParent topo=");
            reply->set_success(true);
            return Status::OK;
        }
        x_ERROR("CoordinatorRPCServer::ReplaceParent failed in addParent");
        reply->set_success(false);
        return Status::CANCELLED;

    } else {
        x_ERROR("CoordinatorRPCServer::ReplaceParent failed in remove parent");
        reply->set_success(false);
        return Status::CANCELLED;
    }
}

Status CoordinatorRPCServer::RemoveParent(ServerContext*, const RemoveParentRequest* request, RemoveParentReply* reply) {
    x_DEBUG("CoordinatorRPCServer::RemoveParent: request = {}", request->DebugString());

    bool success = topologyManagerService->removeParent(request->childid(), request->parentid());
    if (success) {
        x_DEBUG("CoordinatorRPCServer::RemoveParent success");
        reply->set_success(true);
        return Status::OK;
    }
    x_ERROR("CoordinatorRPCServer::RemoveParent failed");
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::NotifyQueryFailure(ServerContext*,
                                                const QueryFailureNotification* request,
                                                QueryFailureNotificationReply* reply) {
    try {
        x_ERROR("CoordinatorRPCServer::notifyQueryFailure: failure message received. id of failed query: {} subplan: {} Id of "
                  "worker: {} Reason for failure: {}",
                  request->queryid(),
                  request->subqueryid(),
                  request->workerid(),
                  request->errormsg());

        x_ASSERT2_FMT(!request->errormsg().empty(),
                        "Cannot fail query without error message " << request->queryid() << " subplan: " << request->subqueryid()
                                                                   << " from worker: " << request->workerid());

        auto sharedQueryId = request->queryid();
        auto subQueryPlanId = request->subqueryid();
        try {
            queryCatalogService->checkAndMarkForFailure(sharedQueryId, subQueryPlanId);
        } catch (std::exception& e) {
            x_ERROR("Unable to mark queries for failure :: subQueryPlanId={}", subQueryPlanId);
            return Status::CANCELLED;
        }

        //Send one failure request for the shared query plan
        if (!queryService->validateAndQueueFailQueryRequest(sharedQueryId, subQueryPlanId, request->errormsg())) {
            x_ERROR("Failed to create Query Failure request for shared query plan {}", sharedQueryId);
            return Status::CANCELLED;
        }

        reply->set_success(true);
        return Status::OK;
    } catch (std::exception& ex) {
        x_ERROR("CoordinatorRPCServer: received broken failure message: {}", ex.what());
        return Status::CANCELLED;
    }
}

Status CoordinatorRPCServer::NotifyEpochTermination(ServerContext*,
                                                    const EpochBarrierPropagationNotification* request,
                                                    EpochBarrierPropagationReply* reply) {
    try {
        x_INFO("CoordinatorRPCServer::propagatePunctuation: received punctuation with timestamp  {} and querySubPlanId {}",
                 request->timestamp(),
                 request->queryid());
        this->replicationService->notifyEpochTermination(request->timestamp(), request->queryid());
        reply->set_success(true);
        return Status::OK;
    } catch (std::exception& ex) {
        x_ERROR("CoordinatorRPCServer: received broken punctuation message: {}", ex.what());
        return Status::CANCELLED;
    }
}

Status CoordinatorRPCServer::GetNodesInRange(ServerContext*, const GetNodesInRangeRequest* request, GetNodesInRangeReply* reply) {

    std::vector<std::pair<uint64_t, x::Spatial::DataTypes::Experimental::GeoLocation>> inRange =
        topologyManagerService->getNodesIdsInRange(x::Spatial::DataTypes::Experimental::GeoLocation(request->geolocation()),
                                                   request->radius());

    for (auto elem : inRange) {
        x::Spatial::Protobuf::WorkerLocationInfo* workerInfo = reply->add_nodes();
        auto geoLocation = elem.second;
        workerInfo->set_id(elem.first);
        auto protoGeoLocation = workerInfo->mutable_geolocation();
        protoGeoLocation->set_lat(geoLocation.getLatitude());
        protoGeoLocation->set_lng(geoLocation.getLongitude());
    }
    return Status::OK;
}

Status CoordinatorRPCServer::SendErrors(ServerContext*, const SendErrorsMessage* request, ErrorReply* reply) {
    try {
        x_ERROR("CoordinatorRPCServer::sendErrors: failure message received."
                  "Id of worker: {} Reason for failure: {}",
                  request->workerid(),
                  request->errormsg());
        // TODO implement here what happens with received Error Messages
        reply->set_success(true);
        return Status::OK;
    } catch (std::exception& ex) {
        x_ERROR("CoordinatorRPCServer: received broken failure message: {}", ex.what());
        return Status::CANCELLED;
    }
}

Status CoordinatorRPCServer::RequestSoftStop(::grpc::ServerContext*,
                                             const ::RequestSoftStopMessage* request,
                                             ::StopRequestReply* response) {
    auto sharedQueryId = request->queryid();
    auto subQueryPlanId = request->subqueryid();
    x_WARNING("CoordinatorRPCServer: received request for soft stopping the shared query plan id: {}", sharedQueryId)

    //Check with query catalog service if the request possible
    auto sourceId = request->sourceid();
    auto softStopPossible = queryCatalogService->checkAndMarkForSoftStop(sharedQueryId, subQueryPlanId, sourceId);

    //Send response
    response->set_success(softStopPossible);
    return Status::OK;
}

Status CoordinatorRPCServer::notifySourceStopTriggered(::grpc::ServerContext*,
                                                       const ::SoftStopTriggeredMessage* request,
                                                       ::SoftStopTriggeredReply* response) {
    auto sharedQueryId = request->queryid();
    auto querySubPlanId = request->querysubplanid();
    x_INFO("CoordinatorRPCServer: received request for soft stopping the sub pan : {}  shared query plan id:{}",
             querySubPlanId,
             sharedQueryId)

    //inform catalog service
    bool success = queryCatalogService->updateQuerySubPlanStatus(sharedQueryId, querySubPlanId, QueryState::SOFT_STOP_TRIGGERED);

    //update response
    response->set_success(success);
    return Status::OK;
}

Status CoordinatorRPCServer::NotifySoftStopCompleted(::grpc::ServerContext*,
                                                     const ::SoftStopCompletionMessage* request,
                                                     ::SoftStopCompletionReply* response) {
    //Fetch the request
    auto queryId = request->queryid();
    auto querySubPlanId = request->querysubplanid();

    //inform catalog service
    bool success = queryCatalogService->updateQuerySubPlanStatus(queryId, querySubPlanId, QueryState::SOFT_STOP_COMPLETED);

    //update response
    response->set_success(success);
    return Status::OK;
}

Status CoordinatorRPCServer::SendScheduledReconnect(ServerContext*,
                                                    const SendScheduledReconnectRequest* request,
                                                    SendScheduledReconnectReply* reply) {
    auto reconnectsToAddMessage = request->addreconnects();
    std::vector<x::Spatial::Mobility::Experimental::ReconnectPoint> addedReconnects;
    for (const auto& toAdd : reconnectsToAddMessage) {
        x::Spatial::DataTypes::Experimental::GeoLocation location(toAdd.geolocation());
        addedReconnects.emplace_back(x::Spatial::Mobility::Experimental::ReconnectPoint{location, toAdd.id(), toAdd.time()});
    }
    auto reconnectsToRemoveMessage = request->removereconnects();
    std::vector<x::Spatial::Mobility::Experimental::ReconnectPoint> removedReconnects;

    for (const auto& toRemove : reconnectsToRemoveMessage) {
        x::Spatial::DataTypes::Experimental::GeoLocation location(toRemove.geolocation());
        removedReconnects.emplace_back(
            x::Spatial::Mobility::Experimental::ReconnectPoint{location, toRemove.id(), toRemove.time()});
    }
    bool success = locationService->updatePredictedReconnect(addedReconnects, removedReconnects);
    reply->set_success(success);
    if (success) {
        return Status::OK;
    }
    return Status::CANCELLED;
}

Status
CoordinatorRPCServer::SendLocationUpdate(ServerContext*, const LocationUpdateRequest* request, LocationUpdateReply* reply) {
    auto coordinates = request->waypoint().geolocation();
    auto timestamp = request->waypoint().timestamp();
    x_DEBUG("Coordinator received location update from node with id {} which reports [{}, {}] at TS {}",
              request->workerid(),
              coordinates.lat(),
              coordinates.lng(),
              timestamp);
    //todo #2862: update coordinator trajectory prediction
    auto geoLocation = x::Spatial::DataTypes::Experimental::GeoLocation(coordinates);
    if (!topologyManagerService->updateGeoLocation(request->workerid(), std::move(geoLocation))) {
        reply->set_success(true);
        return Status::OK;
    }
    reply->set_success(false);
    return Status::CANCELLED;
}

Status CoordinatorRPCServer::GetParents(ServerContext*, const GetParentsRequest* request, GetParentsReply* reply) {
    auto nodeId = request->nodeid();
    auto node = topologyManagerService->findNodeWithId(nodeId);
    auto replyParents = reply->mutable_parentids();
    if (node) {
        auto parents = topologyManagerService->findNodeWithId(nodeId)->getParents();
        replyParents->Reserve(parents.size());
        for (const auto& parent : parents) {
            auto newId = replyParents->Add();
            *newId = parent->as<TopologyNode>()->getId();
        }
        return Status::OK;
    }
    return Status::CANCELLED;
}
