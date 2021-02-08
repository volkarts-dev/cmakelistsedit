add_library(main STATIC "")

target_sources(main PRIVATE
    CMakeListsFile.cpp
    CMakeListsFile.h
)

target_link_libraries(main PUBLIC
    Qt5::Core
    parser
)

target_sources(main PRIVATE
    FileBuffer.cpp
    FileBuffer.h
    DefaultFileBuffer.cpp
    DefaultFileBuffer.h
)


get_target_property(main_src main SOURCES)
source_group("" FILES ${main_src})
