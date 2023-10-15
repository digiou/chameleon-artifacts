# Copyright (C) 2020 by the IoTSPE project (https://IoTSPE.stream)

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#    https://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Set Sources and Headers
add_subdirectory(src)

get_source(x-data-types x_COMPILER_SOURCE_FILES)

# Add Library
add_library(x-data-types SHARED ${x_COMPILER_SOURCE_FILES})
target_link_libraries(x-data-types PUBLIC x-common)
target_include_directories(x-data-types PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/IoTSPE/>)
if (x_ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(x-data-types REUSE_FROM x-common)
endif ()

if (x_ENABLES_TESTS)
    # Add tests with command
    add_subdirectory(tests)
    message(STATUS "Tests are enabled")
else ()
    message(STATUS "Tests are disabled")
endif ()

register_public_header_dir(x-data-types ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/DataTypes Common/DataTypes)
register_public_header_dir(x-data-types ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/ExecutableType Common/ExecutableType)
register_public_header_dir(x-data-types ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/PhysicalTypes Common/PhysicalTypes)
register_public_header_dir(x-data-types ${CMAKE_CURRENT_SOURCE_DIR}/include/API/ API)
register_public_header_file(x-data-types ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/Identifiers.hpp Common/Identifiers.hpp)

#Add the x common and headers to cpack
install(TARGETS x-data-types
        EXPORT IoTSPETargets
        COMPONENT xLibrary
        DESTINATION lib)