qt_add_executable(cli Main.cpp)

target_link_libraries(cli PRIVATE
    project_config
    qt_config
    main
)

set_target_properties(cli PROPERTIES
    OUTPUT_NAME cmle
)
