add_library(main STATIC "")

target_link_libraries(main PUBLIC
    Qt5::Core
    parser
)

get_target_property(main_src main SOURCES)
source_group("" FILES ${main_src})
