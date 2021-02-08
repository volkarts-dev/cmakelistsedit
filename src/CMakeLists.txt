include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(main)

if(CMLE_ENABLE_SAMPLECMD)
    add_subdirectory(cmd)
endif()
