
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#    https://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(FetchContent)
include(cmake/macros.cmake)

# We rely on VCPKG for external dependencies.
# In this script we set up VCPKG depending on the local configuration and system.
# In general, we support three configurations:
# 1. Pre-build dependencies:
#   In this case, we download a archive, which contains all dependencies for IoTSPE.
#   Currently, we provide pre-build dependencies for Ubuntu 20.04 and OSx both for x64 and arm64.
#   If you use another system, the pre-build dependencies could cause undefined errors.
# 2. Local-build dependencies:
#   In this case, we build all dependencies locally as part in the build process.
#   Depending on you local system this may take a while, in particular building clang takes a while.
# 3. Provided dependencies:
#   In this case you provide an own vcpkg toolchain file, such that we use that one to load the dependencies.
#   Please clone https://github.com/IoTSPE/IoTSPE-dependencies and
#   provide the path to the "scripts/buildsystems/vcpkg.cmake" file as -DCMAKE_TOOLCHAIN_FILE=

# Identify the VCPKG_TARGET_TRIPLET depending on the architecture and operating system
# Currently, we support: x64-osx-x, arm64-osx-x, x64-linux-x, arm64-linux-x
if (APPLE)
    if (x_HOST_PROCESSOR MATCHES "x86_64")
        set(VCPKG_TARGET_TRIPLET x64-osx-x)
        set(VCPKG_HOST x64)
    elseif (x_HOST_PROCESSOR MATCHES "arm64")
        set(VCPKG_TARGET_TRIPLET arm64-osx-x)
        set(VCPKG_HOST arm64)
    endif ()
elseif (UNIX AND NOT APPLE)
    if (x_HOST_PROCESSOR MATCHES "x86_64")
        set(VCPKG_HOST x64)
        is_host_musl()
        if (HOST_IS_MUSL)
            set(VCPKG_TARGET_TRIPLET x64-linux-musl-x)
        else ()
            set(VCPKG_TARGET_TRIPLET x64-linux-x)
        endif ()
    elseif (x_HOST_PROCESSOR MATCHES "arm64" OR x_HOST_PROCESSOR MATCHES "aarch64")
        set(VCPKG_TARGET_TRIPLET arm64-linux-x)
        set(VCPKG_HOST arm64)
    endif ()
else ()
    message(FATAL_ERROR "System not supported, currently we only support Linux and OSx")
endif ()
message(STATUS "Use VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")

# In the following we configure vcpkg depending on the selected configuration.
if (CMAKE_TOOLCHAIN_FILE)
    # If a custom toolchain file is provided we use these dependencies.
    # To this end, we have to set the correct x_DEPENDENCIES_BINARY_ROOT.
    message(STATUS "Use provided dependencies with toolchain file: ${CMAKE_TOOLCHAIN_FILE}.")
    cmake_path(GET CMAKE_TOOLCHAIN_FILE PARENT_PATH ParentPath)
    cmake_path(GET ParentPath PARENT_PATH ParentPath)
    cmake_path(GET ParentPath PARENT_PATH ParentPath)
    set(x_DEPENDENCIES_BINARY_ROOT ${ParentPath}/installed/${VCPKG_TARGET_TRIPLET}/)
elseif (x_BUILD_DEPENDENCIES_LOCAL)
    # Build all dependencies locally.
    # To this end, we check out the IoTSPE-dependencies repository and use its manifest file.
    message(STATUS "Build Dependencies locally. This may take a while.")
    FetchContent_Declare(
            xdebs
            GIT_REPOSITORY https://github.com/IoTSPE/IoTSPE-dependencies.git
            GIT_TAG ${VCPKG_BINARY_VERSION}
    )
    FetchContent_Populate(xdebs)
    set(CMAKE_TOOLCHAIN_FILE ${xdebs_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
            CACHE STRING "CMake toolchain file")
    set(VCPKG_MANIFEST_DIR ${xdebs_SOURCE_DIR} CACHE STRING "vcpkg manifest dir")
    set(VCPKG_OVERLAY_TRIPLETS ${xdebs_SOURCE_DIR}/custom-triplets/ CACHE STRING "CMake toolchain file")
    set(VCPKG_OVERLAY_PORTS ${xdebs_SOURCE_DIR}/vcpkg-registry/ports CACHE STRING "VCPKG overlay ports")
    set(x_DEPENDENCIES_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}/vcpkg_installed/${VCPKG_TARGET_TRIPLET})
else (x_USE_PREBUILD_DEPENDENCIES)
    # Use the prebuild dependencies. To this end, we download the correct dependency file from our repository.
    message(STATUS "Use prebuild dependencies")
    set(BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_TARGET_TRIPLET})

    # for x68 linux we currently offer prebuild dependencies for ubuntu 18.04, 20.04, 22.04
    if (${VCPKG_TARGET_TRIPLET} STREQUAL "${VCPKG_HOST}-linux-x")
        get_linux_lsb_release_information()
        message(STATUS "Linux ${LSB_RELEASE_ID_SHORT} ${LSB_RELEASE_VERSION_SHORT} ${LSB_RELEASE_CODENAME_SHORT}")
        set(x_SUPPORTED_UBUNTU_VERSIONS 18.04 20.04 22.04)
        if ((NOT${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu") OR (NOT ${LSB_RELEASE_VERSION_SHORT} IN_LIST x_SUPPORTED_UBUNTU_VERSIONS))
            message(FATAL_ERROR "Currently we only provide pre-build dependencies for Ubuntu: ${x_SUPPORTED_UBUNTU_VERSIONS}. If you use a different linux please build dependencies locally with -Dx_BUILD_DEPENDENCIES_LOCAL=1")
        endif ()
        set(BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_TARGET_TRIPLET})
        set(COMPRESSED_BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_HOST}-linux-ubuntu-${LSB_RELEASE_VERSION_SHORT}-x)
    elseif (${VCPKG_TARGET_TRIPLET} STREQUAL "${VCPKG_HOST}-linux-musl-x")
        set_linux_musl_release_information()
        string(REGEX REPLACE "\.[0-9]+$" "" SHORT_VERSION_ID "${VERSION_ID}")
        message(STATUS "Linux ${ID} ${SHORT_VERSION_ID}")

        set(BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_TARGET_TRIPLET})
        set(COMPRESSED_BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_HOST}-linux-musl-${ID}-${SHORT_VERSION_ID}-x)
    else ()
        set(BINARY_NAME x-dependencies-${VCPKG_BINARY_VERSION}-${VCPKG_TARGET_TRIPLET})
        set(COMPRESSED_BINARY_NAME ${BINARY_NAME})
    endif ()

    set(COMPRESSED_FILE ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.7z)

    IF (NOT EXISTS ${COMPRESSED_FILE})
        message(STATUS "x dependencies do not exist!")
        file(REMOVE ${COMPRESSED_FILE})
        download_file(https://github.com/IoTSPE/dependencies/releases/download/${VCPKG_BINARY_VERSION}/${COMPRESSED_BINARY_NAME}.7z
                ${COMPRESSED_FILE}_tmp)
        file(RENAME ${COMPRESSED_FILE}_tmp ${COMPRESSED_FILE})
    endif ()
    IF (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME})
        message(STATUS "EXTRACT dependencies!")
        file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}_tmp)
        file(ARCHIVE_EXTRACT INPUT ${COMPRESSED_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}_tmp)
        file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}_tmp/${BINARY_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME})
        file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}_tmp)
    endif ()
    # Set toolchain file to use prebuild dependencies.
    message(STATUS "Set toolchain file for prebuild dir.")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}/scripts/buildsystems/vcpkg.cmake")
    set(x_DEPENDENCIES_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}/installed/${VCPKG_TARGET_TRIPLET})
endif ()
message(STATUS "x_DEPENDENCIES_BINARY_ROOT: ${x_DEPENDENCIES_BINARY_ROOT}.")