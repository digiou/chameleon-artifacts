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

#include <Catalogs/Source/PhysicalSourceTypes/MQTTSourceType.hpp>
#include <Util/Logger/Logger.hpp>
#include <string>
#include <utility>

namespace x {

MQTTSourceTypePtr MQTTSourceType::create(Yaml::Node yamlConfig) {
    return std::make_shared<MQTTSourceType>(MQTTSourceType(std::move(yamlConfig)));
}

MQTTSourceTypePtr MQTTSourceType::create(std::map<std::string, std::string> sourceConfigMap) {
    return std::make_shared<MQTTSourceType>(MQTTSourceType(std::move(sourceConfigMap)));
}

MQTTSourceTypePtr MQTTSourceType::create() { return std::make_shared<MQTTSourceType>(MQTTSourceType()); }

MQTTSourceType::MQTTSourceType(std::map<std::string, std::string> sourceConfigMap) : MQTTSourceType() {
    x_INFO("MQTTSourceConfig: Init default MQTT source config object with values from command line args.");

    if (sourceConfigMap.find(Configurations::URL_CONFIG) != sourceConfigMap.end()) {
        url->setValue(sourceConfigMap.find(Configurations::URL_CONFIG)->second);
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no Url defined! Please define a Url.");
    }
    if (sourceConfigMap.find(Configurations::CLIENT_ID_CONFIG) != sourceConfigMap.end()) {
        clientId->setValue(sourceConfigMap.find(Configurations::CLIENT_ID_CONFIG)->second);
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no ClientId defined! Please define a ClientId.");
    }
    if (sourceConfigMap.find(Configurations::USER_NAME_CONFIG) != sourceConfigMap.end()) {
        userName->setValue(sourceConfigMap.find(Configurations::USER_NAME_CONFIG)->second);
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no UserName defined! Please define a UserName.");
    }
    if (sourceConfigMap.find(Configurations::TOPIC_CONFIG) != sourceConfigMap.end()) {
        topic->setValue(sourceConfigMap.find(Configurations::TOPIC_CONFIG)->second);
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no topic defined! Please define a topic.");
    }
    if (sourceConfigMap.find(Configurations::QOS_CONFIG) != sourceConfigMap.end()) {
        qos->setValue(std::stoi(sourceConfigMap.find(Configurations::QOS_CONFIG)->second));
    }
    if (sourceConfigMap.find(Configurations::CLEAN_SESSION_CONFIG) != sourceConfigMap.end()) {
        cleanSession->setValue((sourceConfigMap.find(Configurations::CLEAN_SESSION_CONFIG)->second == "true"));
    }
    if (sourceConfigMap.find(Configurations::FLUSH_INTERVAL_MS_CONFIG) != sourceConfigMap.end()) {
        flushIntervalMS->setValue(std::stof(sourceConfigMap.find(Configurations::FLUSH_INTERVAL_MS_CONFIG)->second));
    }
    if (sourceConfigMap.find(Configurations::INPUT_FORMAT_CONFIG) != sourceConfigMap.end()) {
        inputFormat->setInputFormatEnum(sourceConfigMap.find(Configurations::INPUT_FORMAT_CONFIG)->second);
    }
    if (sourceConfigMap.find(Configurations::SOURCE_GATHERING_INTERVAL_CONFIG) != sourceConfigMap.end()) {
        sourceGatheringInterval->setValue(
            std::stoi(sourceConfigMap.find(Configurations::SOURCE_GATHERING_INTERVAL_CONFIG)->second));
    }
    if (sourceConfigMap.find(Configurations::SOURCE_GATHERING_MODE_CONFIG) != sourceConfigMap.end()) {
        gatheringMode->setValue(
            magic_enum::enum_cast<GatheringMode>(sourceConfigMap.find(Configurations::SOURCE_GATHERING_MODE_CONFIG)->second)
                .value());
    }
    if (sourceConfigMap.find(Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG) != sourceConfigMap.end()) {
        numberOfBuffersToProduce->setValue(
            std::stoi(sourceConfigMap.find(Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG)->second));
    }
    if (sourceConfigMap.find(Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG) != sourceConfigMap.end()) {
        numberOfTuplesToProducePerBuffer->setValue(
            std::stoi(sourceConfigMap.find(Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG)->second));
    }
}

MQTTSourceType::MQTTSourceType(Yaml::Node yamlConfig) : MQTTSourceType() {
    x_INFO("MQTTSourceConfig: Init default MQTT source config object with values from YAML file.");

    if (!yamlConfig[Configurations::URL_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::URL_CONFIG].As<std::string>() != "\n") {
        url->setValue(yamlConfig[Configurations::URL_CONFIG].As<std::string>());
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no Url defined! Please define a Url.");
    }
    if (!yamlConfig[Configurations::CLIENT_ID_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::CLIENT_ID_CONFIG].As<std::string>() != "\n") {
        clientId->setValue(yamlConfig[Configurations::CLIENT_ID_CONFIG].As<std::string>());
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no ClientId defined! Please define a ClientId.");
    }
    if (!yamlConfig[Configurations::USER_NAME_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::USER_NAME_CONFIG].As<std::string>() != "\n") {
        userName->setValue(yamlConfig[Configurations::USER_NAME_CONFIG].As<std::string>());
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no UserName defined! Please define a UserName.");
    }
    if (!yamlConfig[Configurations::TOPIC_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::TOPIC_CONFIG].As<std::string>() != "\n") {
        topic->setValue(yamlConfig[Configurations::TOPIC_CONFIG].As<std::string>());
    } else {
        x_THROW_RUNTIME_ERROR("MQTTSourceConfig:: no topic defined! Please define a topic.");
    }
    if (!yamlConfig[Configurations::QOS_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::QOS_CONFIG].As<std::string>() != "\n") {
        qos->setValue(yamlConfig[Configurations::QOS_CONFIG].As<uint16_t>());
    }
    if (!yamlConfig[Configurations::CLEAN_SESSION_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::CLEAN_SESSION_CONFIG].As<std::string>() != "\n") {
        cleanSession->setValue(yamlConfig[Configurations::CLEAN_SESSION_CONFIG].As<bool>());
    }
    if (!yamlConfig[Configurations::FLUSH_INTERVAL_MS_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::FLUSH_INTERVAL_MS_CONFIG].As<std::string>() != "\n") {
        flushIntervalMS->setValue(std::stof(yamlConfig[Configurations::FLUSH_INTERVAL_MS_CONFIG].As<std::string>()));
    }
    if (!yamlConfig[Configurations::INPUT_FORMAT_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::INPUT_FORMAT_CONFIG].As<std::string>() != "\n") {
        inputFormat->setInputFormatEnum(yamlConfig[Configurations::INPUT_FORMAT_CONFIG].As<std::string>());
    }
    if (!yamlConfig[Configurations::SOURCE_GATHERING_INTERVAL_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::SOURCE_GATHERING_INTERVAL_CONFIG].As<std::string>() != "\n") {
        sourceGatheringInterval->setValue(yamlConfig[Configurations::SOURCE_GATHERING_INTERVAL_CONFIG].As<uint32_t>());
    }
    if (!yamlConfig[Configurations::SOURCE_GATHERING_MODE_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::SOURCE_GATHERING_MODE_CONFIG].As<std::string>() != "\n") {
        gatheringMode->setValue(
            magic_enum::enum_cast<GatheringMode>(yamlConfig[Configurations::SOURCE_GATHERING_MODE_CONFIG].As<std::string>())
                .value());
    }
    if (!yamlConfig[Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG].As<std::string>() != "\n") {
        numberOfBuffersToProduce->setValue(yamlConfig[Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG].As<uint32_t>());
    }
    if (!yamlConfig[Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG].As<std::string>().empty()
        && yamlConfig[Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG].As<std::string>() != "\n") {
        numberOfTuplesToProducePerBuffer->setValue(
            yamlConfig[Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG].As<uint32_t>());
    }
}

MQTTSourceType::MQTTSourceType()
    : PhysicalSourceType(SourceType::MQTT_SOURCE),
      url(Configurations::ConfigurationOption<std::string>::create(
          Configurations::URL_CONFIG,
          "ws://127.0.0.1:9001",
          "url to connect to needed for: MQTTSource, ZMQSource, OPCSource, KafkaSource")),
      clientId(Configurations::ConfigurationOption<std::string>::create(
          Configurations::CLIENT_ID_CONFIG,
          "testClient",
          "clientId, needed for: MQTTSource (needs to be unique for each connected "
          "MQTTSource), KafkaSource (use this for groupId)")),
      userName(Configurations::ConfigurationOption<std::string>::create(
          Configurations::USER_NAME_CONFIG,
          "testUser",
          "userName, needed for: MQTTSource (can be chosen arbitrary), OPCSource")),
      topic(Configurations::ConfigurationOption<std::string>::create(Configurations::TOPIC_CONFIG,
                                                                     "demoTownSensorData",
                                                                     "topic to listen to, needed for: MQTTSource, KafkaSource")),
      qos(Configurations::ConfigurationOption<uint32_t>::create(Configurations::QOS_CONFIG,
                                                                2,
                                                                "quality of service, needed for: MQTTSource")),
      cleanSession(Configurations::ConfigurationOption<bool>::create(
          Configurations::CLEAN_SESSION_CONFIG,
          true,
          "cleanSession true = clean up session after client loses connection, false = keep data for "
          "client after connection loss (persistent session), needed for: MQTTSource")),
      flushIntervalMS(Configurations::ConfigurationOption<float>::create("flushIntervalMS",
                                                                         -1,
                                                                         "tupleBuffer flush interval in milliseconds")),
      inputFormat(Configurations::ConfigurationOption<Configurations::InputFormat>::create(Configurations::INPUT_FORMAT_CONFIG,
                                                                                           Configurations::InputFormat::JSON,
                                                                                           "input data format")),
      sourceGatheringInterval(
          Configurations::ConfigurationOption<uint32_t>::create(Configurations::SOURCE_GATHERING_INTERVAL_CONFIG,
                                                                0,
                                                                "Gathering interval of the source.")),
      gatheringMode(Configurations::ConfigurationOption<GatheringMode>::create(Configurations::SOURCE_GATHERING_MODE_CONFIG,
                                                                               GatheringMode::INTERVAL_MODE,
                                                                               "Gathering mode of the source.")),
      numberOfBuffersToProduce(
          Configurations::ConfigurationOption<uint32_t>::create(Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG,
                                                                0,
                                                                "Number of buffers to produce.")),
      numberOfTuplesToProducePerBuffer(
          Configurations::ConfigurationOption<uint32_t>::create(Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG,
                                                                0,
                                                                "Number of tuples to produce per buffer.")) {
    x_INFO("xSourceConfig: Init source config object with default values.");
}

std::string MQTTSourceType::toString() {
    std::stringstream ss;
    ss << "MQTTSourceType => {\n";
    ss << Configurations::URL_CONFIG + ":" + url->toStringNameCurrentValue();
    ss << Configurations::CLIENT_ID_CONFIG + ":" + clientId->toStringNameCurrentValue();
    ss << Configurations::USER_NAME_CONFIG + ":" + userName->toStringNameCurrentValue();
    ss << Configurations::TOPIC_CONFIG + ":" + topic->toStringNameCurrentValue();
    ss << Configurations::QOS_CONFIG + ":" + qos->toStringNameCurrentValue();
    ss << Configurations::CLEAN_SESSION_CONFIG + ":" + cleanSession->toStringNameCurrentValue();
    ss << Configurations::FLUSH_INTERVAL_MS_CONFIG + ":" + flushIntervalMS->toStringNameCurrentValue();
    ss << Configurations::INPUT_FORMAT_CONFIG + ":" + inputFormat->toStringNameCurrentValueEnum();
    ss << Configurations::SOURCE_GATHERING_INTERVAL_CONFIG + ":" + sourceGatheringInterval->toStringNameCurrentValue();
    ss << Configurations::SOURCE_GATHERING_MODE_CONFIG + ":" + std::string(magic_enum::enum_name(gatheringMode->getValue()));
    ss << Configurations::NUMBER_OF_BUFFERS_TO_PRODUCE_CONFIG + ":" + numberOfBuffersToProduce->toStringNameCurrentValue();
    ss << Configurations::NUMBER_OF_TUPLES_TO_PRODUCE_PER_BUFFER_CONFIG + ":"
            + numberOfTuplesToProducePerBuffer->toStringNameCurrentValue();
    ss << "\n}";
    return ss.str();
}

bool MQTTSourceType::equal(const PhysicalSourceTypePtr& other) {
    if (!other->instanceOf<MQTTSourceType>()) {
        return false;
    }
    auto otherSourceConfig = other->as<MQTTSourceType>();
    return url->getValue() == otherSourceConfig->url->getValue()
        && clientId->getValue() == otherSourceConfig->clientId->getValue()
        && userName->getValue() == otherSourceConfig->userName->getValue()
        && topic->getValue() == otherSourceConfig->topic->getValue() && qos->getValue() == otherSourceConfig->qos->getValue()
        && cleanSession->getValue() == otherSourceConfig->cleanSession->getValue()
        && flushIntervalMS->getValue() == otherSourceConfig->flushIntervalMS->getValue()
        && inputFormat->getValue() == otherSourceConfig->inputFormat->getValue()
        && sourceGatheringInterval->getValue() == otherSourceConfig->sourceGatheringInterval->getValue()
        && gatheringMode->getValue() == otherSourceConfig->gatheringMode->getValue()
        && numberOfBuffersToProduce->getValue() == otherSourceConfig->numberOfBuffersToProduce->getValue()
        && numberOfTuplesToProducePerBuffer->getValue() == otherSourceConfig->numberOfTuplesToProducePerBuffer->getValue();
}

Configurations::StringConfigOption MQTTSourceType::getUrl() const { return url; }

Configurations::StringConfigOption MQTTSourceType::getClientId() const { return clientId; }

Configurations::StringConfigOption MQTTSourceType::getUserName() const { return userName; }

Configurations::StringConfigOption MQTTSourceType::getTopic() const { return topic; }

Configurations::IntConfigOption MQTTSourceType::getQos() const { return qos; }

Configurations::BoolConfigOption MQTTSourceType::getCleanSession() const { return cleanSession; }

Configurations::FloatConfigOption MQTTSourceType::getFlushIntervalMS() const { return flushIntervalMS; }

Configurations::InputFormatConfigOption MQTTSourceType::getInputFormat() const { return inputFormat; }

Configurations::IntConfigOption MQTTSourceType::getGatheringInterval() const { return sourceGatheringInterval; }

Configurations::GatheringModeConfigOption MQTTSourceType::getGatheringMode() const { return gatheringMode; }

Configurations::IntConfigOption MQTTSourceType::getNumberOfBuffersToProduce() const { return numberOfBuffersToProduce; }

Configurations::IntConfigOption MQTTSourceType::getNumberOfTuplesToProducePerBuffer() const {
    return numberOfTuplesToProducePerBuffer;
}

void MQTTSourceType::setUrl(std::string urlValue) { url->setValue(std::move(urlValue)); }

void MQTTSourceType::setClientId(std::string clientIdValue) { clientId->setValue(std::move(clientIdValue)); }

void MQTTSourceType::setUserName(std::string userNameValue) { userName->setValue(std::move(userNameValue)); }

void MQTTSourceType::setTopic(std::string topicValue) { topic->setValue(std::move(topicValue)); }

void MQTTSourceType::setQos(uint32_t qosValue) { qos->setValue(qosValue); }

void MQTTSourceType::setCleanSession(bool cleanSessionValue) { cleanSession->setValue(cleanSessionValue); }

void MQTTSourceType::setFlushIntervalMS(float flushIntervalMs) { flushIntervalMS->setValue(flushIntervalMs); }

void MQTTSourceType::setInputFormat(std::string inputFormatValue) {
    inputFormat->setInputFormatEnum(std::move(inputFormatValue));
}

void MQTTSourceType::setInputFormat(Configurations::InputFormat inputFormatValue) {
    inputFormat->setValue(std::move(inputFormatValue));
}

void MQTTSourceType::setGatheringInterval(uint32_t sourceGatheringIntervalValue) {
    sourceGatheringInterval->setValue(sourceGatheringIntervalValue);
}

void MQTTSourceType::setGatheringMode(std::string inputGatheringMode) {
    MQTTSourceType::setGatheringMode(magic_enum::enum_cast<GatheringMode>(inputGatheringMode).value());
}

void MQTTSourceType::setGatheringMode(GatheringMode inputGatheringMode) { gatheringMode->setValue(inputGatheringMode); }

void MQTTSourceType::setNumberOfBuffersToProduce(uint32_t numberOfBuffersToProduceValue) {
    numberOfBuffersToProduce->setValue(numberOfBuffersToProduceValue);
}

void MQTTSourceType::setNumberOfTuplesToProducePerBuffer(uint32_t numberOfTuplesToProducePerBufferValue) {
    numberOfTuplesToProducePerBuffer->setValue(numberOfTuplesToProducePerBufferValue);
}

void MQTTSourceType::reset() {
    setUrl(url->getDefaultValue());
    setClientId(clientId->getDefaultValue());
    setUserName(userName->getDefaultValue());
    setTopic(topic->getDefaultValue());
    setQos(qos->getDefaultValue());
    setCleanSession(cleanSession->getDefaultValue());
    setFlushIntervalMS(flushIntervalMS->getDefaultValue());
    setInputFormat(inputFormat->getDefaultValue());
    setGatheringInterval(sourceGatheringInterval->getDefaultValue());
    setNumberOfBuffersToProduce(numberOfBuffersToProduce->getDefaultValue());
    setNumberOfTuplesToProducePerBuffer(numberOfTuplesToProducePerBuffer->getDefaultValue());
}

}// namespace x
