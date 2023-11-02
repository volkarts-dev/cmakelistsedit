cmake_minimum_required(VERSION 3.16)

project(CMakeListsEdit LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

option(CMLE_ENABLE_CLI "Build the command line interface" ON)
option(CMLE_ENABLE_UNITTESTS "Build unit tests" ON)
option(CMLE_ENABLE_CODECOVERAGE "Build unit tests with code coverage" OFF)

include(PreventInSourceBuilds)
include(CompilerWarnings)
include(QtFunctions)

qt_load_packages()

set(CMAKE_AUTOMOC ON)

add_library(project_config INTERFACE)
set_compiler_warnings(project_config INTERFACE)
target_compile_features(project_config INTERFACE
    cxx_std_17
)
set_target_properties(project_config PROPERTIES
    CXX_EXTENSIONS OFF
)

add_subdirectory(src)

if(CMLE_ENABLE_UNITTESTS)
    qt_load_test_packages()

    enable_testing()

    add_subdirectory(tests)
endif()
