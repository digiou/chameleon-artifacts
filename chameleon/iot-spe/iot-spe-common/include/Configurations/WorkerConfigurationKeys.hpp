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

#ifndef x_COMMON_INCLUDE_CONFIGURATIONS_WORKERCONFIGURATIONKEYS_HPP_
#define x_COMMON_INCLUDE_CONFIGURATIONS_WORKERCONFIGURATIONKEYS_HPP_

#include <string>

namespace x::Worker::Configuration {
const std::string TENSORFLOW_SUPPORT = "TENSOR_FLOW";
const std::string JAVA_UDF_SUPPORT = "JAVA_UDF";
const std::string MOBILITY_SUPPORT = "MOBILITY";
const std::string SPATIAL_SUPPORT = "SPATIAL_TYPE";
}// namespace x::Worker::Configuration

#endif// x_COMMON_INCLUDE_CONFIGURATIONS_WORKERCONFIGURATIONKEYS_HPP_
