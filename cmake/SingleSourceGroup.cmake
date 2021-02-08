macro(single_source_group target_name)
    get_target_property(${target_name}_src ${target_name} SOURCES)
    source_group("" FILES ${${target_name}_src})
endmacro()
