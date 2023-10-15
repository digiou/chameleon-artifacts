
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
        FileSink.cpp
        ZmqSink.cpp
        SinkMedium.cpp
        PrintSink.cpp
        NullOutputSink.cpp
        MaterializedViewSink.cpp
        MonitoringSink.cpp
        KafkaSink.cpp
)


if (x_USE_OPC)
    add_source_files(x-core
        OPCSink.cpp
    )
endif ()

if (x_USE_MQTT)
    add_source_files(x-core
            MQTTSink.cpp
    )
endif ()