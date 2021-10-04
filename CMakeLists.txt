cmake_minimum_required(VERSION 3.13)

project(CMakeListsEdit LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

option(CMLE_ENABLE_SAMPLECMD "Build the sample command line utility" ON)
option(CMLE_ENABLE_UNITTESTS "Build unit tests" ON)
option(CMLE_ENABLE_CODECOVERAGE "Build unit tests with code coverage" OFF)

include(SingleSourceGroup)

find_package(Qt5 COMPONENTS Core REQUIRED)
set(CMAKE_AUTOMOC ON)

add_subdirectory(src)

if(CMLE_ENABLE_UNITTESTS)
    enable_testing()

    find_package(Qt5 COMPONENTS Test REQUIRED)

    add_subdirectory(tests)
endif()
