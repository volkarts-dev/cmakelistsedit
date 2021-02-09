add_library(main STATIC "")

target_sources(main
    PRIVATE
    Atest1.cpp)

target_link_libraries(main PUBLIC
    Qt5::Core
    parser
)

target_sources(main PRIVATE
    DefaultFileBuffer.cpp
    DefaultFileBuffer.h
    FileBuffer.cpp
    FileBuffer.h
)


get_target_property(main_src main SOURCES)
source_group("" FILES ${main_src})
