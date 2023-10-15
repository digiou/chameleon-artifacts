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

get_source(x-common x_COMMON_SOURCE_FILES)

# Add Library
add_library(x-common SHARED ${x_COMMON_SOURCE_FILES})
target_link_libraries(x-common PUBLIC ${LIBRARIES})
target_include_directories(x-common PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/IoTSPE/>)

set_property(TARGET x-common PROPERTY INTERFACE_LINK_LIBRARIES
        $<BUILD_INTERFACE:${LIBRARIES}>
        $<INSTALL_INTERFACE:>)


register_public_header_dir(x-common ${CMAKE_CURRENT_SOURCE_DIR}/include/Util/ Util)
register_public_header_dir(x-common ${CMAKE_CURRENT_SOURCE_DIR}/include/Exceptions/ Exceptions)
if (x_ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(x-common PRIVATE "include/PrecompiledHeaders.hpp")
endif ()

if (x_ENABLES_TESTS)
    # Add tests with command
    add_subdirectory(tests)
    message(STATUS "Tests are enabled")
else ()
    message(STATUS "Tests are disabled")
endif ()

#Add the x common and headers to cpack
install(TARGETS x-common
        EXPORT IoTSPETargets
        COMPONENT xLibrary
        DESTINATION lib)