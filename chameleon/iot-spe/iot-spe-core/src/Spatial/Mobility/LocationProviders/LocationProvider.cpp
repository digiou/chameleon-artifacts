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

#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Spatial/DataTypes/GeoLocation.hpp>
#include <Spatial/DataTypes/Waypoint.hpp>
#include <Spatial/Mobility/LocationProviders/LocationProvider.hpp>
#include <Spatial/Mobility/LocationProviders/LocationProviderCSV.hpp>
#include <Util/Experimental/LocationProviderType.hpp>
#include <Util/Experimental/SpatialType.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Spatial::Mobility::Experimental {

LocationProvider::LocationProvider(Spatial::Experimental::SpatialType spatialType,
                                   DataTypes::Experimental::GeoLocation geoLocation)
    : workerGeoLocation(geoLocation), spatialType(spatialType) {}

Spatial::Experimental::SpatialType LocationProvider::getSpatialType() const { return spatialType; };

DataTypes::Experimental::Waypoint LocationProvider::getCurrentWaypoint() {
    switch (spatialType) {
        case Spatial::Experimental::SpatialType::MOBILE_NODE: return getCurrentWaypoint();
        case Spatial::Experimental::SpatialType::FIXED_LOCATION: return DataTypes::Experimental::Waypoint(workerGeoLocation);
        case Spatial::Experimental::SpatialType::NO_LOCATION:
        case Spatial::Experimental::SpatialType::INVALID:
            x_WARNING("Location Provider has invalid spatial type")
            return DataTypes::Experimental::Waypoint(DataTypes::Experimental::Waypoint::invalid());
    }
}

LocationProviderPtr LocationProvider::create(Configurations::WorkerConfigurationPtr workerConfig) {
    x::Spatial::Mobility::Experimental::LocationProviderPtr locationProvider;

    switch (workerConfig->mobilityConfiguration.locationProviderType.getValue()) {
        case x::Spatial::Mobility::Experimental::LocationProviderType::BASE:
            locationProvider =
                std::make_shared<x::Spatial::Mobility::Experimental::LocationProvider>(workerConfig->nodeSpatialType,
                                                                                         workerConfig->locationCoordinates);
            x_INFO("creating base location provider")
            break;
        case x::Spatial::Mobility::Experimental::LocationProviderType::CSV:
            if (workerConfig->mobilityConfiguration.locationProviderConfig.getValue().empty()) {
                x_FATAL_ERROR("cannot create csv location provider if no provider config is set");
                exit(EXIT_FAILURE);
            }
            locationProvider = std::make_shared<x::Spatial::Mobility::Experimental::LocationProviderCSV>(
                workerConfig->mobilityConfiguration.locationProviderConfig,
                workerConfig->mobilityConfiguration.locationProviderSimulatedStartTime);
            break;
        case x::Spatial::Mobility::Experimental::LocationProviderType::INVALID:
            x_FATAL_ERROR("Trying to create location provider but provider type is invalid");
            exit(EXIT_FAILURE);
    }

    return locationProvider;
}
}// namespace x::Spatial::Mobility::Experimental