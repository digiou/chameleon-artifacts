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

#include <Catalogs/Source/SourceCatalog.hpp>
#include <Catalogs/UDF/UDFCatalog.hpp>
#include <Components/xCoordinator.hpp>
#include <REST/Controller/ConnectivityController.hpp>
#include <REST/Controller/LocationController.hpp>
#include <REST/Controller/MonitoringController.hpp>
#include <REST/Controller/QueryCatalogController.hpp>
#include <REST/Controller/QueryController.hpp>
#include <REST/Controller/SourceCatalogController.hpp>
#include <REST/Controller/TopologyController.hpp>
#include <REST/Controller/UdfCatalogController.hpp>
#include <REST/Handlers/ErrorHandler.hpp>
#include <REST/RestServer.hpp>
#include <REST/RestServerInterruptHandler.hpp>
#include <Util/Logger/Logger.hpp>
#include <iostream>
#include <oatpp/network/Address.hpp>
#include <oatpp/network/Server.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>
#include <oatpp/web/server/interceptor/AllowCorsGlobal.hpp>
#include <utility>

namespace x {

RestServer::RestServer(std::string host,
                       uint16_t port,
                       xCoordinatorWeakPtr coordinator,
                       QueryCatalogServicePtr queryCatalogService,
                       SourceCatalogServicePtr sourceCatalogService,
                       TopologyManagerServicePtr topologyManagerService,
                       GlobalExecutionPlanPtr globalExecutionPlan,
                       QueryServicePtr queryService,
                       MonitoringServicePtr monitoringService,
                       GlobalQueryPlanPtr globalQueryPlan,
                       Catalogs::UDF::UDFCatalogPtr udfCatalog,
                       Runtime::BufferManagerPtr bufferManager,
                       LocationServicePtr locationService,
                       std::optional<std::string> corsAllowedOrigin)
    : host(std::move(host)), port(port), coordinator(std::move(coordinator)), queryCatalogService(std::move(queryCatalogService)),
      globalExecutionPlan(std::move(globalExecutionPlan)), queryService(std::move(queryService)),
      globalQueryPlan(std::move(globalQueryPlan)), sourceCatalogService(std::move(sourceCatalogService)),
      topologyManagerService(std::move(topologyManagerService)), udfCatalog(std::move(udfCatalog)),
      locationService(std::move(locationService)), monitoringService(std::move(monitoringService)),
      bufferManager(std::move(bufferManager)), corsAllowedOrigin(std::move(corsAllowedOrigin)) {}

bool RestServer::start() {
    x_INFO("Starting Oatpp Server on {}:{}", host, std::to_string(port));
    RestServerInterruptHandler::hookUserInterruptHandler();
    try {
        // Initialize Oatpp Environment
        oatpp::base::Environment::init();
        //Creates component parts necessary to start server, then starts a blocking server
        run();
        // Destroy Oatpp Environment
        oatpp::base::Environment::destroy();
    } catch (const std::exception& e) {
        x_ERROR("RestServer: Unable to start REST server [{}:{}] {}", host, std::to_string(port), e.what());
        return false;
    } catch (...) {
        x_FATAL_ERROR("RestServer: Unable to start REST server unknown exception.");
        return false;
    }
    return true;
}

bool RestServer::stop() {
    std::unique_lock lock(mutex);
    stopRequested = true;
    return stopRequested;
}

void RestServer::run() {

    /* create serializer and deserializer configurations */
    auto serializeConfig = oatpp::parser::json::mapping::Serializer::Config::createShared();
    auto deserializeConfig = oatpp::parser::json::mapping::Deserializer::Config::createShared();

    /* enable beautifier */
    serializeConfig->useBeautifier = true;

    /* Initialize Object mapper */
    auto objectMapper = oatpp::parser::json::mapping::ObjectMapper::createShared(serializeConfig, deserializeConfig);

    /* Initialize Error Handler */
    ErrorHandlerPtr errorHandler = std::make_shared<ErrorHandler>(objectMapper);

    /* Create Router for HTTP requests routing */
    auto router = oatpp::web::server::HttpRouter::createShared();

    /* Create controllers and add all of their endpoints to the router */
    auto connectivityController = REST::Controller::ConnectivityController::create(objectMapper, "/connectivity");
    auto queryCatalogController = REST::Controller::QueryCatalogController::create(objectMapper,
                                                                                   queryCatalogService,
                                                                                   coordinator,
                                                                                   globalQueryPlan,
                                                                                   "/queryCatalog",
                                                                                   errorHandler);
    auto topologyController =
        REST::Controller::TopologyController::create(objectMapper, topologyManagerService, "/topology", errorHandler);
    auto queryController = REST::Controller::QueryController::create(objectMapper,
                                                                     queryService,
                                                                     queryCatalogService,
                                                                     globalExecutionPlan,
                                                                     "/query",
                                                                     errorHandler);
    auto udfCatalogController =
        REST::Controller::UDFCatalogController::create(objectMapper, udfCatalog, "/udfCatalog", errorHandler);
    auto sourceCatalogController =
        REST::Controller::SourceCatalogController::create(objectMapper, sourceCatalogService, errorHandler, "/sourceCatalog");
    auto locationController =
        REST::Controller::LocationController::create(objectMapper, locationService, "/location", errorHandler);
    auto monitoringController = REST::Controller::MonitoringController::create(objectMapper,
                                                                               monitoringService,
                                                                               bufferManager,
                                                                               errorHandler,
                                                                               "/monitoring");

    router->addController(connectivityController);
    router->addController(queryCatalogController);
    router->addController(queryController);
    router->addController(topologyController);
    router->addController(sourceCatalogController);
    router->addController(udfCatalogController);
    router->addController(locationController);
    router->addController(connectivityController);
    router->addController(queryCatalogController);
    router->addController(monitoringController);

    /* Create HTTP connection handler with router */
    auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);
    //register error handler
    connectionHandler->setErrorHandler(std::make_shared<ErrorHandler>(objectMapper));

    /* Add CORS-enabling interceptors */
    if (corsAllowedOrigin.has_value()) {
        connectionHandler->addRequestInterceptor(std::make_shared<oatpp::web::server::interceptor::AllowOptionsGlobal>());
        connectionHandler->addResponseInterceptor(
            std::make_shared<oatpp::web::server::interceptor::AllowCorsGlobal>(corsAllowedOrigin.value()));
    }

    /* Create TCP connection provider */
    auto connectionProvider =
        oatpp::network::tcp::server::ConnectionProvider::createShared({host, port, oatpp::network::Address::IP_4});

    /* Create a server, which takes provided TCP connections and passes them to HTTP connection handler. */
    oatpp::network::Server server(connectionProvider, connectionHandler);

    /* Print info about server port */
    x_INFO("IoTSPE REST Server listening on port {}", port);

    /* Run server */
    server.run([this]() -> bool {
        x_DEBUG("checking if stop request has arrived for rest server listening on port {}.", port);
        std::unique_lock lock(mutex);
        return !stopRequested;
    });
    connectionProvider->stop();
    connectionHandler->stop();
}

}// namespace x
