include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_BINARY_DIR}/../src
)

macro(simple_test name)
    add_executable(test_${name} test_${name}.cpp)

    target_link_libraries(test_${name} PRIVATE
        project_config
        qt_config
        Qt5::Test
        ${ARGN}
    )

    target_compile_definitions(test_${name} PRIVATE
        RESOURCE_DIR="${CMAKE_SOURCE_DIR}/tests/res"
    )
endmacro()

if(CMLE_ENABLE_CODECOVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_lcov(
        NAME codecoveragereport
        EXECUTABLE ctest
        BASE_DIRECTORY ${CMAKE_SOURCE_DIR}
        EXCLUDE "/usr/*" "tests/*" "build/*"
    )
endif()


add_subdirectory(cmd)

add_subdirectory(main)
