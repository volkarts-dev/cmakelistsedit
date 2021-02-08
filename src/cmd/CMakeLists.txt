add_library(cmd_lib "")

target_sources(cmd_lib PRIVATE
    DefaultFileBuffer.cpp
    DefaultFileBuffer.h
)

target_link_libraries(cmd_lib PUBLIC
    main
)

set_target_properties(cmd_lib PROPERTIES
    RUNTIME_OUTPUT_NAME cmedit
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

single_source_group(cmd_lib)

add_executable(cmd Main.cpp)
target_link_libraries(cmd PRIVATE cmd_lib)
single_source_group(cmd)
