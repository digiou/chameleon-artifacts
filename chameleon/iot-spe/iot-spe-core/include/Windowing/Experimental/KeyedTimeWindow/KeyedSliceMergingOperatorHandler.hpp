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

#ifndef x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_KEYEDTIMEWINDOW_KEYEDSLICEMERGINGOPERATORHANDLER_HPP_
#define x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_KEYEDTIMEWINDOW_KEYEDSLICEMERGINGOPERATORHANDLER_HPP_
#include <Runtime/Execution/OperatorHandler.hpp>
#include <Windowing/WindowingForwardRefs.hpp>
#include <atomic>
#include <ctype.h>
#include <memory>

namespace x::Experimental {
class HashMapFactory;
using HashMapFactoryPtr = std::shared_ptr<HashMapFactory>;
class LockFreeMultiOriginWatermarkProcessor;
}// namespace x::Experimental

namespace x::Windowing::Experimental {
class KeyedSlice;
class SliceMergeTask;
using KeyedSlicePtr = std::unique_ptr<KeyedSlice>;
class KeyedGlobalSliceStore;
class KeyedSliceStaging;

/**
 * @brief The KeyedSliceMergingOperatorHandler merges thread local pre-aggregated slices for keyed
 * tumbling and sliding window aggregations.
 */
class KeyedSliceMergingOperatorHandler : public Runtime::Execution::OperatorHandler,
                                         public detail::virtual_enable_shared_from_this<KeyedSliceMergingOperatorHandler, false> {
    using inherited0 = detail::virtual_enable_shared_from_this<KeyedSliceMergingOperatorHandler, false>;
    using inherited1 = Runtime::Reconfigurable;

  public:
    KeyedSliceMergingOperatorHandler(const Windowing::LogicalWindowDefinitionPtr& windowDefinition);

    void setup(Runtime::Execution::PipelineExecutionContext& ctx, x::Experimental::HashMapFactoryPtr hashmapFactory);

    /**
     * @brief Get a reference to the slice staging.
     * @note This should be only called from the generated code.
     * @return KeyedSliceStaging
     */
    inline KeyedSliceStaging& getSliceStaging() { return *sliceStaging.get(); }

    /**
     * @brief Gets a weak pointer to the slice staging
     * @return std::weak_ptr<KeyedSliceStaging>
     */
    std::weak_ptr<KeyedSliceStaging> getSliceStagingPtr();

    void start(Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext,
               Runtime::StateManagerPtr stateManager,
               uint32_t localStateVariableId) override;

    void stop(Runtime::QueryTerminationType queryTerminationType,
              Runtime::Execution::PipelineExecutionContextPtr pipelineExecutionContext) override;

    /**
     * @brief Creates a new keyed slice for a specific slice merge task
     * @param sliceMergeTaskS liceMergeTask
     * @return KeyedSlicePtr
     */
    KeyedSlicePtr createKeyedSlice(SliceMergeTask* sliceMergeTask);

    /**
     * @brief Gets the window definition
     * @return Windowing::LogicalWindowDefinitionPtr
     */
    Windowing::LogicalWindowDefinitionPtr getWindowDefinition();

    void postReconfigurationCallback(Runtime::ReconfigurationMessage& message) override;

    ~KeyedSliceMergingOperatorHandler();

  private:
    std::atomic<uint32_t> activeCounter;
    uint64_t windowSize;
    uint64_t windowSlide;
    std::shared_ptr<KeyedSliceStaging> sliceStaging;
    std::weak_ptr<KeyedGlobalSliceStore> globalSliceStore;
    Windowing::LogicalWindowDefinitionPtr windowDefinition;
    x::Experimental::HashMapFactoryPtr factory;
};

}// namespace x::Windowing::Experimental

#endif// x_CORE_INCLUDE_WINDOWING_EXPERIMENTAL_KEYEDTIMEWINDOW_KEYEDSLICEMERGINGOPERATORHANDLER_HPP_
