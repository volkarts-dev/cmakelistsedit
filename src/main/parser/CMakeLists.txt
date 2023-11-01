qt_add_library(parser STATIC
    CMakeFileContent.cpp
    CMakeFileContent.h
    CMakeListsParser.cpp
    CMakeListsParser.h
    cmListFileLexer.c
    cmListFileLexer.h
    cmStandardLexer.h
)

target_link_libraries(parser PUBLIC
    Qt::Core
)
