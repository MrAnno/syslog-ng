set(AFSTREAMS_SOURCES
    afstreams.c
    afstreams.h
    afstreams-parser.c
    afstreams-parser.h
    afstreams-plugin.c
    ${CMAKE_CURRENT_BINARY_DIR}/afstreams-grammar.h
    ${CMAKE_CURRENT_BINARY_DIR}/afstreams-grammar.c
)

option(ENABLE_SUN_STREAMS "Enable Sun Streams source" ON)

include (CheckIncludeFile)
check_include_file(sys/strlog.h HAVE_STRLOG)
check_include_file(stropts.h HAVE_STROPTS)

if(HAVE_STRLOG AND HAVE_STROPTS AND ENABLE_SUN_STREAMS)
    set(BUILD_AFSTREAMS ON)
else()
    set(BUILD_AFSTREAMS OFF)
endif()

if (BUILD_AFSTREAMS)
    generate_y_from_ym(modules/afstreams/afstreams-grammar)

    bison_target(StreamsGrammar
        ${CMAKE_CURRENT_BINARY_DIR}/afstreams-grammar.y
        ${CMAKE_CURRENT_BINARY_DIR}/afstreams-grammar.c
        COMPILE_FLAGS ${BISON_FLAGS})

    add_library(afstreams MODULE ${AFSTREAMS_SOURCES})
    target_include_directories (afstreams PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(afstreams PRIVATE syslog-ng)
    
    install(TARGETS afstreams LIBRARY DESTINATION lib/syslog-ng/)
endif()
