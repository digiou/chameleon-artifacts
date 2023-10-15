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
#ifndef x_CORE_INCLUDE_SOURCES_MATERIALIZEDVIEWSOURCE_HPP_
#define x_CORE_INCLUDE_SOURCES_MATERIALIZEDVIEWSOURCE_HPP_

#include <Sources/DataSource.hpp>

namespace x::Experimental::MaterializedView {

// Forward decl.
class MaterializedView;
using MaterializedViewPtr = std::shared_ptr<MaterializedView>;
class MaterializedViewSource;
using MaterializedViewSourcePtr = std::shared_ptr<MaterializedViewSource>;

/**
 * @brief this class provides a materialized view as a data source
 */
class MaterializedViewSource : public DataSource {

  public:
    /*
     * @brief The constructor of a materialized view Source
     * @param schema of the source
     * @param bufferManager pointer to the buffer manager
     * @param queryManager pointer to the query manager
     * @param operatorId current operator id
     * @param originId represents the identifier of the upstream operator that represents the origin of the input stream
     * @param numSourceLocalBuffers the number of buffers allocated to a source
     * @param gatheringMode
     * @param physicalSourceName
     * @param successors
     * @param view
     **/
    MaterializedViewSource(SchemaPtr schema,
                           Runtime::BufferManagerPtr bufferManager,
                           Runtime::QueryManagerPtr queryManager,
                           OperatorId operatorId,
                           OriginId originId,
                           size_t numSourceLocalBuffers,
                           GatheringMode gatheringMode,
                           const std::string& physicalSourceName,
                           std::vector<Runtime::Execution::SuccessorExecutablePipeline> successors,
                           MaterializedViewPtr view);

    /**
     * @brief method to receive a buffer from the materialized view source
     * @return TupleBufferPtr containing the received buffer
     */
    std::optional<Runtime::TupleBuffer> receiveData() override;

    /**
     * @brief Provides a string representation of the source
     * @return The string representation of the source
     */
    std::string toString() const override;

    /**
     * @brief Provides the type of the source
     * @return the type of the source
     */
    SourceType getType() const override;

    /**
     *  @brief Provides the id of the used materialized view
     *  @return materialized view id
     */
    uint64_t getViewId() const;

  private:
    MaterializedViewPtr view;

};// class MaterializedViewSource
}// namespace x::Experimental::MaterializedView
#endif// x_CORE_INCLUDE_SOURCES_MATERIALIZEDVIEWSOURCE_HPP_
