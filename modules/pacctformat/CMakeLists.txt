if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set (ENABLE_PACCT_DEFAULT ON)
else()
    set (ENABLE_PACCT_DEFAULT OFF)
endif()

option(ENABLE_PACCT "Enable support for reading Process Accounting files (EXPERIMENTAL, Linux only)." ${ENABLE_PACCT_DEFAULT})

if (ENABLE_PACCT)
    set(PACCT_FORMAT_SOURCES
        pacct-format.c
        pacct-format.h
        pacct-format-plugin.c
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    add_library(pacctformat MODULE ${PACCT_FORMAT_SOURCES})
    target_link_libraries(pacctformat PRIVATE syslog-ng)
    
    install(TARGETS pacctformat LIBRARY DESTINATION lib/syslog-ng/)
endif()
