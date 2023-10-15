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

#include <Runtime/xThread.hpp>

namespace x::Runtime {

/// The first thread will have index 0.
std::atomic<uint32_t> xThread::next_index{0};

/// No thread IDs have been used yet.
std::atomic<bool> xThread::id_used[MaxNumThreads] = {};

/// Give the new thread an ID.
thread_local xThread::ThreadId xThread::id{};

}// namespace x::Runtime