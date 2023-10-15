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

#ifndef x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SINKS_FILESINKDESCRIPTOR_HPP_
#define x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SINKS_FILESINKDESCRIPTOR_HPP_

#include <Operators/LogicalOperators/Sinks/SinkDescriptor.hpp>
#include <Util/FaultToleranceType.hpp>
#include <string>

class SinkMedium;
namespace x {

/**
 * @brief Descriptor defining properties used for creating physical file sink
 */
class FileSinkDescriptor : public SinkDescriptor {
  public:
    /**
     * @brief Factory method to create a new file sink descriptor
     * @param fileName the path to the output file
     * @param sinkFormat the sink format
     * @param append flag to indicate if to append to file
     * @param addTimestamp flat to indicate if timestamp shall be add when writing to file
     * @param faultToleranceType: fault tolerance type of a query
     * @param numberOfOrigins: number of origins of a given query
     * @return descriptor for file sink
     */
    static SinkDescriptorPtr create(std::string fileName,
                                    std::string sinkFormat,
                                    const std::string& append,
                                    bool addTimestamp,
                                    FaultToleranceType faultToleranceType,
                                    uint64_t numberOfOrigins);

    /**
     * @brief Factory method to create a new file sink descriptor
     * @param fileName the path to the output file
     * @param sinkFormat the sink format
     * @param append flag to indicate if to append to file
     * @param addTimestamp flat to indicate if timestamp shall be add when writing to file
     * @return descriptor for file sink
     */
    static SinkDescriptorPtr create(std::string fileName, std::string sinkFormat, const std::string& append, bool addTimestamp);

    /**
     * @brief Factory method to create a new file sink descriptor
     * @param fileName the path to the output file
     * @param sinkFormat the sink format
     * @param append flag to indicate if to append to file
     * @return descriptor for file sink
     */
    static SinkDescriptorPtr create(std::string fileName, std::string sinkFormat, const std::string& append);

    /**
     * @brief Factory method to create a new file sink descriptor as default
     * @param filePath the path to the output file
     * @param addTimestamp flag to add timestamp
     * @return descriptor for file sink
     */
    static SinkDescriptorPtr create(std::string fileName, bool addTimestamp);

    /**
     * @brief Factory method to create a new file sink descriptor as default
     * @param filePath the path to the output file
     * @return descriptor for file sink
     */
    static SinkDescriptorPtr create(std::string fileName);

    /**
     * @brief Get the file name where the data is to be written
     */
    const std::string& getFileName() const;

    std::string toString() const override;
    [[nodiscard]] bool equal(SinkDescriptorPtr const& other) override;

    std::string getSinkFormatAsString() const;

    bool getAppend() const;

  private:
    explicit FileSinkDescriptor(std::string fileName,
                                std::string sinkFormat,
                                bool append,
                                bool addTimestamp,
                                FaultToleranceType faultToleranceType,
                                uint64_t numberOfOrigins);
    std::string fileName;
    std::string sinkFormat;
    bool append;
};

using FileSinkDescriptorPtr = std::shared_ptr<FileSinkDescriptor>;
}// namespace x

#endif// x_CORE_INCLUDE_OPERATORS_LOGICALOPERATORS_SINKS_FILESINKDESCRIPTOR_HPP_
