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

#include <Execution/Operators/Streaming/Join/StreamHashJoin/DataStructure/FixedPagesLinkedList.hpp>
#include <Execution/Operators/Streaming/Join/StreamHashJoin/DataStructure/GlobalHashTableLocking.hpp>
#include <Execution/Operators/Streaming/Join/StreamJoinUtil.hpp>
#include <Util/Common.hpp>
#include <zlib.h>

namespace x::Runtime::Execution::Operators {

GlobalHashTableLocking::GlobalHashTableLocking(size_t sizeOfRecord,
                                               size_t numPartitions,
                                               FixedPagesAllocator& fixedPagesAllocator,
                                               size_t pageSize,
                                               size_t preAllocPageSizeCnt)
    : StreamJoinHashTable(sizeOfRecord, numPartitions, fixedPagesAllocator, pageSize, preAllocPageSizeCnt) {}

uint8_t* GlobalHashTableLocking::insert(uint64_t key) const {
    auto hashedKey = x::Util::murmurHash(key);
    x_TRACE("into key={} bucket={}", key, getBucketPos(hashedKey));
    return buckets[getBucketPos(hashedKey)]->appendConcurrentUsingLocking(hashedKey);
}

}// namespace x::Runtime::Execution::Operators