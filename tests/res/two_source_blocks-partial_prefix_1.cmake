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
    abc/DefaultFileBuffer.cpp
    abc/DefaultFileBuffer.h
    def/xyz/DefaultFileBuffer.cpp
    def/xyz/DefaultFileBuffer.h
    abc/xyz/Atest1.cpp
)


get_target_property(main_src main SOURCES)
source_group("" FILES ${main_src})
