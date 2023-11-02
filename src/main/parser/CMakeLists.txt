add_library(cmake_lexer STATIC
    cmake/cmListFileLexer.cxx
    cmake/cmListFileLexer.h
    cmake/cmListFileLexer.in.l
    cmake/cmStandardLexer.h
)

qt_add_library(parser STATIC
    CMakeFileContent.cpp
    CMakeFileContent.h
    CMakeListsParser.cpp
    CMakeListsParser.h
)

target_link_libraries(parser PRIVATE
    project_config
    qt_config
    cmake_lexer
)

target_link_libraries(parser PUBLIC
    Qt::Core
)
