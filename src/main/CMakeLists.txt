qt_add_library(main STATIC
    include/cmle/CMakeListsFile.h
    include/cmle/FileBuffer.h
    include/cmle/StandardFileBuffer.h
    CMakeListsFile.cpp
    StandardFileBuffer.cpp
    CMakeListsFile_p.h
)

add_library(cmle::cmle ALIAS main)

qt_configure_mocs(main)

target_link_libraries(main PRIVATE
    project_config
    qt_config
)

target_link_libraries(main PUBLIC
    parser
    Qt::Core
)

target_include_directories(main SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(parser)
