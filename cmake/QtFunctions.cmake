set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(qt_config INTERFACE)
target_compile_definitions(qt_config INTERFACE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

function(qt_configure_mocs target)
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/mocs_compilation.cpp
        PROPERTIES
            COMPILE_FLAGS "-Wno-useless-cast"
    )
endfunction()

macro(qt_load_packages)
    find_package(Qt5 COMPONENTS Core REQUIRED)
endmacro()

macro(qt_load_test_packages)
    find_package(Qt5 COMPONENTS Test REQUIRED)
endmacro()
