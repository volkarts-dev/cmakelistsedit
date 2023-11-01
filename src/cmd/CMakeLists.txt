add_executable(cmd Main.cpp)

target_link_libraries(cmd PRIVATE
    project_config
    qt_config
    main
)
