if (x_CODE_COVERAGE)
    set(CODE_COVERAGE ON)
endif ()

function(add_x_common_test)
    add_executable(${ARGN})
    set(TARGET_NAME ${ARGV0})
    target_link_libraries(${TARGET_NAME} x-test-util)
    if (x_ENABLE_PRECOMPILED_HEADERS)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM x-common)
        # We need to compile with -fPIC to include with x-common compiled headers as it uses PIC
        target_compile_options(${TARGET_NAME} PUBLIC "-fPIC")
    endif ()
    if (CODE_COVERAGE)
        target_code_coverage(${TARGET_NAME} PUBLIC AUTO ALL EXTERNAL OBJECTS x x-compiler x-common EXCLUDE tests/*)
        message(STATUS "Added cc test ${TARGET_NAME}")
    endif ()
    gtest_discover_tests(${TARGET_NAME})
    message(STATUS "Added ut test ${TARGET_NAME}")
endfunction()

function(add_compiler_unit_test)
    add_executable(${ARGN})
    set(TARGET_NAME ${ARGV0})
    target_link_libraries(${TARGET_NAME} x-compiler x-test-util)
    if (x_ENABLE_PRECOMPILED_HEADERS)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM x-common)
    endif ()
    if (CODE_COVERAGE)
        target_code_coverage(${TARGET_NAME} PUBLIC AUTO ALL EXTERNAL OBJECTS x x-compiler x-common EXCLUDE tests/*)
        message(STATUS "Added cc test ${TARGET_NAME}")
    endif ()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    message(STATUS "Added ut test ${TARGET_NAME}")
endfunction()

function(add_x_unit_test)
    add_executable(${ARGN})
    set(TARGET_NAME ${ARGV0})
    target_link_libraries(${TARGET_NAME} x-core-test-util)
    if (x_ENABLE_PRECOMPILED_HEADERS)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM x-common)
    endif ()
    if (CODE_COVERAGE)
        target_code_coverage(${TARGET_NAME} PUBLIC AUTO ALL EXTERNAL OBJECTS x x-compiler x-common EXCLUDE tests/*)
        message(STATUS "Added cc test ${TARGET_NAME}")
    endif ()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    message(STATUS "Added ut test ${TARGET_NAME}")
endfunction()

function(add_x_integration_test)
    # create a test executable that may contain multiple source files.
    # first param is TARGET_NAME
    add_executable(${ARGN})
    set(TARGET_NAME ${ARGV0})
    if (x_ENABLE_PRECOMPILED_HEADERS)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM x-common)
    endif ()
    target_link_libraries(${TARGET_NAME} x-core-test-util)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    message(STATUS "Added it test ${TARGET_NAME}")
endfunction()

function(enable_extra_test_features)
    if (CODE_COVERAGE)
        add_code_coverage_all_targets(EXCLUDE tests/*)
    endif ()
endfunction()

function(instrument_codebase)
    if (CODE_COVERAGE)
        target_code_coverage(x PUBLIC AUTO)
        target_code_coverage(xWorker PUBLIC AUTO)
        target_code_coverage(xCoordinator PUBLIC AUTO)
        target_code_coverage(x-compiler PUBLIC AUTO)
        target_code_coverage(x-common PUBLIC AUTO)
        target_code_coverage(x-client PUBLIC AUTO)
        target_code_coverage(x-runtime PUBLIC AUTO)
        target_code_coverage(x-data-types PUBLIC AUTO)
    endif ()
endfunction()