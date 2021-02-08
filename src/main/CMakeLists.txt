add_library(main STATIC "")
add_library(cmle::cmle ALIAS main)

target_sources(main PRIVATE
    CMakeListsFile.cpp
    CMakeListsFile.h
    FileBuffer.cpp
    FileBuffer.h
)

target_link_libraries(main PUBLIC
    Qt5::Core
    parser
)

target_include_directories(main INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

single_source_group(main)

add_subdirectory(parser)
