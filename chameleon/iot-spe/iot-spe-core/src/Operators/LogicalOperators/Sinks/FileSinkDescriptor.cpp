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

#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/OperatorNode.hpp>
#include <utility>

namespace x {

SinkDescriptorPtr FileSinkDescriptor::create(std::string fileName) { return create(std::move(fileName), false); }

SinkDescriptorPtr FileSinkDescriptor::create(std::string fileName, bool addTimestamp) {
    return create(std::move(fileName), "CSV_FORMAT", "OVERWRITE", addTimestamp);
}

SinkDescriptorPtr
FileSinkDescriptor::create(std::string fileName, std::string sinkFormat, const std::string& append, bool addTimestamp) {
    return create(std::move(fileName), std::move(sinkFormat), append, addTimestamp, FaultToleranceType::NONE, 1);
}

SinkDescriptorPtr FileSinkDescriptor::create(std::string fileName,
                                             std::string sinkFormat,
                                             const std::string& append,
                                             bool addTimestamp,
                                             FaultToleranceType faultToleranceType,
                                             uint64_t numberOfOrigins) {
    return std::make_shared<FileSinkDescriptor>(FileSinkDescriptor(std::move(fileName),
                                                                   std::move(sinkFormat),
                                                                   append == "APPEND",
                                                                   addTimestamp,
                                                                   faultToleranceType,
                                                                   numberOfOrigins));
}

SinkDescriptorPtr FileSinkDescriptor::create(std::string fileName, std::string sinkFormat, const std::string& append) {
    return create(std::move(fileName), std::move(sinkFormat), append, false, FaultToleranceType::NONE, 1);
}

FileSinkDescriptor::FileSinkDescriptor(std::string fileName,
                                       std::string sinkFormat,
                                       bool append,
                                       bool addTimestamp,
                                       FaultToleranceType faultToleranceType,
                                       uint64_t numberOfOrigins)
    : SinkDescriptor(faultToleranceType, numberOfOrigins, addTimestamp), fileName(std::move(fileName)),
      sinkFormat(std::move(sinkFormat)), append(append) {}

const std::string& FileSinkDescriptor::getFileName() const { return fileName; }

std::string FileSinkDescriptor::toString() const { return "FileSinkDescriptor()"; }

bool FileSinkDescriptor::equal(SinkDescriptorPtr const& other) {
    if (!other->instanceOf<FileSinkDescriptor>()) {
        return false;
    }
    auto otherSinkDescriptor = other->as<FileSinkDescriptor>();
    return fileName == otherSinkDescriptor->fileName;
}

bool FileSinkDescriptor::getAppend() const { return append; }

std::string FileSinkDescriptor::getSinkFormatAsString() const { return sinkFormat; }

}// namespace x