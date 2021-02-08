add_library(parser STATIC "")

target_sources(parser PRIVATE
    CMakeFileContent.cpp
    CMakeFileContent.h
    CMakeListsParser.cpp
    CMakeListsParser.h
    cmListFileLexer.c
    cmListFileLexer.h
    cmStandardLexer.h
)

target_link_libraries(parser PUBLIC
    Qt5::Core
)

single_source_group(parser)
