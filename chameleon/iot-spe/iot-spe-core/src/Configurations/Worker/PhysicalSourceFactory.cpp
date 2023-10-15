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

#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/ArrowSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/BinarySourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/DefaultSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/KafkaSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/MQTTSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/OPCSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/PhysicalSourceType.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/SenseSourceType.hpp>
#include <Configurations/ConfigurationOption.hpp>
#include <Configurations/Worker/PhysicalSourceFactory.hpp>
#include <Util/Logger/Logger.hpp>

namespace x {

namespace Configurations {

PhysicalSourcePtr PhysicalSourceFactory::createFromString(std::string, std::map<std::string, std::string>& commandLineParams) {

    std::string sourceType, logicalSourceName, physicalSourceName;
    for (auto it = commandLineParams.begin(); it != commandLineParams.end(); ++it) {
        if (it->first == SOURCE_TYPE_CONFIG && !it->second.empty()) {
            sourceType = it->second;
        } else if (it->first == PHYSICAL_SOURCE_NAME_CONFIG && !it->second.empty()) {
            physicalSourceName = it->second;
        } else if (it->first == LOGICAL_SOURCE_NAME_CONFIG && !it->second.empty()) {
            logicalSourceName = it->second;
        }
    }

    if (logicalSourceName.empty()) {
        x_WARNING("No logical source name is not supplied for creating the physical source. Please supply "
                    "logical source name using --{}",
                    LOGICAL_SOURCE_NAME_CONFIG);
        return nullptr;
    } else if (physicalSourceName.empty()) {
        x_WARNING(
            "No physical source name supplied for creating the physical source. Please supply physical source name using --{}",
            PHYSICAL_SOURCE_NAME_CONFIG);
        return nullptr;
    } else if (sourceType.empty()) {
        x_WARNING("No source type supplied for creating the physical source. Please supply source type using --{}",
                    SOURCE_TYPE_CONFIG);
        return nullptr;
    }

    auto physicalSourceType = createPhysicalSourceType(sourceType, commandLineParams);
    return PhysicalSource::create(logicalSourceName, physicalSourceName, physicalSourceType);
}

PhysicalSourcePtr PhysicalSourceFactory::createFromYaml(Yaml::Node& yamlConfig) {
    std::vector<PhysicalSourcePtr> physicalSources;
    //Iterate over all physical sources defined in the yaml file
    std::string logicalSourceName, physicalSourceName, sourceType;
    if (!yamlConfig[LOGICAL_SOURCE_NAME_CONFIG].As<std::string>().empty()
        && yamlConfig[LOGICAL_SOURCE_NAME_CONFIG].As<std::string>() != "\n") {
        logicalSourceName = yamlConfig[LOGICAL_SOURCE_NAME_CONFIG].As<std::string>();
    } else {
        x_THROW_RUNTIME_ERROR("Found Invalid Logical Source Configuration. Please define Logical Source Name.");
    }

    if (!yamlConfig[PHYSICAL_SOURCE_NAME_CONFIG].As<std::string>().empty()
        && yamlConfig[PHYSICAL_SOURCE_NAME_CONFIG].As<std::string>() != "\n") {
        physicalSourceName = yamlConfig[PHYSICAL_SOURCE_NAME_CONFIG].As<std::string>();
    } else {
        x_THROW_RUNTIME_ERROR("Found Invalid Physical Source Configuration. Please define Physical Source Name.");
    }

    if (!yamlConfig[SOURCE_TYPE_CONFIG].As<std::string>().empty() && yamlConfig[SOURCE_TYPE_CONFIG].As<std::string>() != "\n") {
        sourceType = yamlConfig[SOURCE_TYPE_CONFIG].As<std::string>();
    } else {
        x_THROW_RUNTIME_ERROR("Found Invalid Physical Source Configuration. Please define Source type.");
    }

    if (yamlConfig[PHYSICAL_SOURCE_TYPE_CONFIGURATION].IsMap()) {
        auto physicalSourceTypeConfiguration = yamlConfig[PHYSICAL_SOURCE_TYPE_CONFIGURATION];
        auto physicalSourceType = createPhysicalSourceType(sourceType, physicalSourceTypeConfiguration);
        return PhysicalSource::create(logicalSourceName, physicalSourceName, physicalSourceType);
    } else {
        x_THROW_RUNTIME_ERROR(
            "Found Invalid Physical Source Configuration. Please define Source Type Configuration properties.");
    }
}

PhysicalSourceTypePtr
PhysicalSourceFactory::createPhysicalSourceType(std::string sourceType,
                                                const std::map<std::string, std::string>& commandLineParams) {
    switch (magic_enum::enum_cast<SourceType>(sourceType).value()) {
        case SourceType::CSV_SOURCE: return CSVSourceType::create(commandLineParams);
        case SourceType::MQTT_SOURCE: return MQTTSourceType::create(commandLineParams);
        case SourceType::KAFKA_SOURCE: return KafkaSourceType::create(commandLineParams);
        case SourceType::OPC_SOURCE: return OPCSourceType::create(commandLineParams);
        case SourceType::BINARY_SOURCE: return BinarySourceType::create(commandLineParams);
        case SourceType::SENSE_SOURCE: return SenseSourceType::create(commandLineParams);
        case SourceType::DEFAULT_SOURCE: return DefaultSourceType::create(commandLineParams);
        case SourceType::MATERIALIZEDVIEW_SOURCE: return DefaultSourceType::create(commandLineParams);
        case SourceType::ARROW_SOURCE: return ArrowSourceType::create(commandLineParams);
        default: x_THROW_RUNTIME_ERROR("SourceConfigFactory:: source type " << sourceType << " not supported");
    }
}

PhysicalSourceTypePtr PhysicalSourceFactory::createPhysicalSourceType(std::string sourceType, Yaml::Node& yamlConfig) {

    if (!magic_enum::enum_cast<SourceType>(sourceType).has_value()) {
        x_THROW_RUNTIME_ERROR("SourceConfigFactory:: source type " << sourceType << " not supported");
    }

    switch (magic_enum::enum_cast<SourceType>(sourceType).value()) {
        case SourceType::CSV_SOURCE: return CSVSourceType::create(yamlConfig);
        case SourceType::MQTT_SOURCE: return MQTTSourceType::create(yamlConfig);
        case SourceType::KAFKA_SOURCE: return KafkaSourceType::create(yamlConfig);
        case SourceType::OPC_SOURCE: return OPCSourceType::create(yamlConfig);
        case SourceType::BINARY_SOURCE: return BinarySourceType::create(yamlConfig);
        case SourceType::SENSE_SOURCE: return SenseSourceType::create(yamlConfig);
        case SourceType::ARROW_SOURCE: return ArrowSourceType::create(yamlConfig);
        case SourceType::DEFAULT_SOURCE: return DefaultSourceType::create(yamlConfig);
        default: x_THROW_RUNTIME_ERROR("SourceConfigFactory:: source type " << sourceType << " not supported");
    }
}

}// namespace Configurations
}// namespace x