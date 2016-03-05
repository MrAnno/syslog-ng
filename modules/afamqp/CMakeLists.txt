FIND_PACKAGE(PkgConfig REQUIRED)
pkg_search_module(LIBRABBITMQ REQUIRED librabbitmq>=0.5.3)

if(LIBRABBITMQ_FOUND)

    set(AFAMQP_HEADERS
        "${CMAKE_CURRENT_BINARY_DIR}/afamqp-grammar.h"
        "afamqp-parser.h"
        "afamqp.h"
        )

    set(AFAMQP_SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/afamqp-grammar.c"
        "afamqp-parser.c"
        "afamqp.c"
        )

    generate_y_from_ym(modules/afamqp/afamqp-grammar)

    bison_target(AfamqpGrammar
        ${CMAKE_CURRENT_BINARY_DIR}/afamqp-grammar.y
        ${CMAKE_CURRENT_BINARY_DIR}/afamqp-grammar.c
        COMPILE_FLAGS ${BISON_FLAGS})

    include_directories (${CMAKE_CURRENT_BINARY_DIR})
    include_directories (${CMAKE_CURRENT_SOURCE_DIR})
    add_library(afamqp MODULE ${AFAMQP_SOURCES})

    target_link_libraries(afamqp ${LIBRABBITMQ_LIBRARIES})
    target_include_directories(afamqp PUBLIC ${LIBRABBITMQ_INCLUDE_DIRS})
    target_compile_options(afamqp PUBLIC ${LIBRABBITMQ_CFLAGS_OTHER})

    target_link_libraries(afamqp PRIVATE syslog-ng)

    install(TARGETS afamqp LIBRARY DESTINATION lib/syslog-ng/)
endif(LIBRABBITMQ_FOUND)
