
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#    https://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_source_files(x-core
        SourceDescriptor.cpp
        SourceLogicalOperatorNode.cpp
        SenseSourceDescriptor.cpp
        KafkaSourceDescriptor.cpp
        ZmqSourceDescriptor.cpp
        BinarySourceDescriptor.cpp
        CsvSourceDescriptor.cpp
        DefaultSourceDescriptor.cpp
        LogicalSourceDescriptor.cpp
        MonitoringSourceDescriptor.cpp
        NetworkSourceDescriptor.cpp
        OPCSourceDescriptor.cpp
        MemorySourceDescriptor.cpp
        StaticDataSourceDescriptor.cpp
        MQTTSourceDescriptor.cpp
        LambdaSourceDescriptor.cpp
        BenchmarkSourceDescriptor.cpp
        MaterializedViewSourceDescriptor.cpp
        TCPSourceDescriptor.cpp
        ArrowSourceDescriptor.cpp
)
