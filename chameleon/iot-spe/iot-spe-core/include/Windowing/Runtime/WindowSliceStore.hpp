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

#ifndef x_CORE_INCLUDE_WINDOWING_RUNTIME_WINDOWSLICESTORE_HPP_
#define x_CORE_INCLUDE_WINDOWING_RUNTIME_WINDOWSLICESTORE_HPP_
#include <Util/Logger/Logger.hpp>
#include <Windowing/Runtime/SliceMetaData.hpp>
#include <mutex>
namespace x::Windowing {

/**
 * @brief The WindowSliceStore stores slices consisting of metadata and a partial aggregate.
 * @tparam PartialAggregateType type of the partial aggregate
 */
template<class PartialAggregateType>
class WindowSliceStore {
  public:
    explicit WindowSliceStore(PartialAggregateType value)
        : defaultValue(value), partialAggregates(std::vector<PartialAggregateType>()) {}

    ~WindowSliceStore() { x_TRACE("~WindowSliceStore()"); }

    /**
    * @brief Get the corresponding slide index for a particular timestamp ts.
    * @param ts timestamp of the record.
    * @return the index of a slice. If not found it returns -1.
    */
    inline uint64_t getSliceIndexByTs(const uint64_t ts) {
        for (uint64_t i = 0; i < sliceMetaData.size(); i++) {
            auto slice = sliceMetaData[i];
            if (slice.getStartTs() <= ts && slice.getEndTs() > ts) {
                x_TRACE("getSliceIndexByTs for ts={} return index={}", ts, i);
                return i;
            }
        }
        auto lastSlice = sliceMetaData.back();
        x_ERROR("getSliceIndexByTs for could not find a slice, this should not happen. current ts {} last slice {} - {}",
                  ts,
                  lastSlice.getStartTs(),
                  lastSlice.getEndTs());
        x_THROW_RUNTIME_ERROR("getSliceIndexByTs for could not find a slice, this should not happen ts"
                                << ts << " last slice " << lastSlice.getStartTs() << " - " << lastSlice.getEndTs());
        //TODO: change this back once we have the vector clocks
        return 0;
    }

    /**
     * @brief Appends a new slice to the meta data vector and intitalises a new partial aggregate with the default value.
     * @param slice
     */
    inline void appendSlice(SliceMetaData slice) {
        x_TRACE("appendSlice start={} end={}", slice.getStartTs(), slice.getEndTs());
        sliceMetaData.push_back(slice);
        partialAggregates.push_back(defaultValue);
    }

    /**
    * @brief Prepend a new slice to the meta data vector and intitalises a new partial aggregate with the default value.
    * @param slice
    */
    inline void prependSlice(SliceMetaData slice) {
        x_TRACE("prependSlice start={} end={}", slice.getStartTs(), slice.getEndTs());
        sliceMetaData.emplace(sliceMetaData.begin(), slice);
        partialAggregates.emplace(partialAggregates.begin(), defaultValue);
    }

    /**
     * @return most current slice'index.
     */
    inline uint64_t getCurrentSliceIndex() { return sliceMetaData.size() - 1; }

    /**
     * @brief this method increment the numbner of tuples per slice for the slice at idx
     * @param slideIdx
     */
    inline void incrementRecordCnt(uint64_t slideIdx) { sliceMetaData[slideIdx].incrementRecordsPerSlice(); }

    /**
     * @brief Remove slices between index 0 and pos.
     * @param pos the position till we want to remove slices.
     */
    inline void removeSlicesUntil(uint64_t watermark) {
        auto itSlice = sliceMetaData.begin();
        auto itAggs = partialAggregates.begin();
        for (; itSlice != sliceMetaData.end(); ++itSlice) {
            if (itSlice->getEndTs() > watermark) {
                break;
            }
            x_TRACE("WindowSliceStore removeSlicesUntil: watermark={} from slice endts={} sliceMetaData size={} "
                      "partialaggregate size={}",
                      watermark,
                      itSlice->getEndTs(),
                      sliceMetaData.size(),
                      partialAggregates.size());
            itAggs++;
        }

        sliceMetaData.erase(sliceMetaData.begin(), itSlice);
        partialAggregates.erase(partialAggregates.begin(), itAggs);
        x_TRACE("WindowSliceStore: removeSlicesUntil size after cleanup slice={} aggs={}",
                  sliceMetaData.size(),
                  partialAggregates.size());
    }

    /**
     * @brief Checks if the slice store is empty.
     * @return true if empty.
     */
    inline uint64_t empty() { return sliceMetaData.empty(); }

    /**
     * @brief Gets the slice meta data.
     * @return vector of slice meta data.
     */
    inline std::vector<SliceMetaData>& getSliceMetadata() { return sliceMetaData; }

    /**
     * @brief Gets partial aggregates.
     * @return vector of partial aggregates.
     */
    inline std::vector<PartialAggregateType>& getPartialAggregates() { return partialAggregates; }

    std::mutex& mutex() const { return storeMutex; }

    const PartialAggregateType defaultValue;
    uint64_t nextEdge = 0;

  private:
    std::vector<SliceMetaData> sliceMetaData;
    std::vector<PartialAggregateType> partialAggregates;
    mutable std::mutex storeMutex;
};

}// namespace x::Windowing

#endif// x_CORE_INCLUDE_WINDOWING_RUNTIME_WINDOWSLICESTORE_HPP_
