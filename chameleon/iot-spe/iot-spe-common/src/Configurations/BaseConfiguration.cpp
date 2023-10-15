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

#include <Configurations/BaseConfiguration.hpp>
#include <filesystem>
#include <fstream>

namespace x::Configurations {

BaseConfiguration::BaseConfiguration() : BaseOption(){};

BaseConfiguration::BaseConfiguration(const std::string& name, const std::string& description) : BaseOption(name, description){};

void BaseConfiguration::parseFromYAMLNode(const Yaml::Node config) {
    auto optionMap = getOptionMap();
    if (!config.IsMap()) {
        throw ConfigurationException("Malformed YAML configuration file");
    }
    for (auto entry = config.Begin(); entry != config.End(); entry++) {
        auto identifier = (*entry).first;
        auto node = (*entry).second;
        if (!optionMap.contains(identifier)) {
            throw ConfigurationException("Identifier: " + identifier
                                         + " is not known. Check if it exposed in the getOptions function.");
        }
        // check if config is empty
        if (!config.As<std::string>().empty() && config.As<std::string>() != "\n") {
            throw ConfigurationException("Value for " + identifier + " is empty.");
        }
        optionMap[identifier]->parseFromYAMLNode(node);
    }
}

void BaseConfiguration::parseFromString(std::string identifier, std::map<std::string, std::string>& inputParams) {
    auto optionMap = getOptionMap();

    if (!optionMap.contains(identifier)) {
        throw ConfigurationException("Identifier " + identifier + " is not known.");
    }
    auto option = optionMap[identifier];
    if (dynamic_cast<BaseConfiguration*>(option)) {
        dynamic_cast<BaseConfiguration*>(optionMap[identifier])->overwriteConfigWithCommandLineInput(inputParams);
    } else {
        optionMap[identifier]->parseFromString(identifier, inputParams);
    }
}

void BaseConfiguration::overwriteConfigWithYAMLFileInput(const std::string& filePath) {
    try {
        Yaml::Node config;
        Yaml::Parse(config, filePath.c_str());
        if (config.IsNone()) {
            return;
        }
        parseFromYAMLNode(config);
    } catch (std::exception& ex) {
        throw ConfigurationException("Exception while loading configurations from " + filePath + ". Exception: " + ex.what());
    }
}

void BaseConfiguration::overwriteConfigWithCommandLineInput(const std::map<std::string, std::string>& inputParams) {
    std::map<std::string, std::map<std::string, std::string>> groupedIdentifiers;
    for (auto parm = inputParams.begin(); parm != inputParams.end(); ++parm) {
        auto identifier = parm->first;
        auto value = parm->second;
        const std::string identifierStart = "--";
        if (identifier.starts_with(identifierStart)) {
            // remove the -- in the beginning
            identifier = identifier.substr(identifierStart.size());
        }

        if (identifier.find('.') != std::string::npos) {
            auto index = std::string(identifier).find('.');
            auto parentIdentifier = std::string(identifier).substr(0, index);
            auto childrenIdentifier = std::string(identifier).substr(index + 1, identifier.length());
            groupedIdentifiers[parentIdentifier].insert({childrenIdentifier, value});
        } else {
            groupedIdentifiers[identifier].insert({identifier, value});
        }
    }

    for (auto [identifier, values] : groupedIdentifiers) {
        parseFromString(identifier, values);
    }
}

std::string BaseConfiguration::toString() {
    std::stringstream ss;
    for (auto option : getOptions()) {
        ss << option->toString() << "\n";
    }
    return ss.str();
}

void BaseConfiguration::clear() {
    for (auto* option : getOptions()) {
        option->clear();
    }
};

std::map<std::string, Configurations::BaseOption*> BaseConfiguration::getOptionMap() {
    std::map<std::string, Configurations::BaseOption*> optionMap;
    for (auto* option : getOptions()) {
        auto identifier = option->getName();
        optionMap[identifier] = option;
    }
    return optionMap;
}

bool BaseConfiguration::persistWorkerIdInYamlConfigFile(std::string yamlFilePath, uint64_t workerId, bool withOverwrite) {
    std::ifstream configFile(yamlFilePath);
    std::stringstream ss;
    std::string searchKey = "workerId: ";

    if (!withOverwrite) {
        std::string yamlValueAsString = std::to_string(workerId);
        std::string yamlConfigValue = "\n" + searchKey + yamlValueAsString;

        if (!yamlFilePath.empty() && std::filesystem::exists(yamlFilePath)) {
            configFile >> ss.rdbuf();
            try {
                std::ofstream output;
                output.open(yamlFilePath, std::ios::app);// append mode
                output << yamlConfigValue;
            } catch (std::exception& e) {
                throw ConfigurationException("Exception while persisting in yaml file", e.what());
            }
        }
    } else {
        ss << configFile.rdbuf();
        std::string yamlContent = ss.str();

        size_t startPos = yamlContent.find(searchKey);
        if (startPos != std::string::npos) {
            // move the position to the start of the value
            startPos += searchKey.size();
            // find the end of the line
            size_t endPos = yamlContent.find('\n', startPos);
            // replace the old value with the new value for workerId
            yamlContent.replace(startPos, endPos - startPos, std::to_string(workerId));
        } else {
            return false;
        }

        std::ofstream output(yamlFilePath);
        output << yamlContent;
    }
    configFile.close();
    return true;
}

}// namespace x::Configurations