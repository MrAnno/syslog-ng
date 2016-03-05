
FIND_PACKAGE(PkgConfig REQUIRED)
pkg_search_module(LIBMONGOC REQUIRED libmongoc>=1.1)

if(LIBMONGOC_FOUND)

    set(AFMONGODB_HEADERS
        "${CMAKE_CURRENT_BINARY_DIR}/afmongodb-grammar.h"
        "afmongodb.h"
        "afmongodb-parser.h"
        )

    set(AFMONGODB_SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/afmongodb-grammar.c"
        "afmongodb.c"
        "afmongodb-parser.c"
        )

    generate_y_from_ym(modules/afmongodb/afmongodb-grammar)

    bison_target(AfmongodbGrammar
        ${CMAKE_CURRENT_BINARY_DIR}/afmongodb-grammar.y
        ${CMAKE_CURRENT_BINARY_DIR}/afmongodb-grammar.c
        COMPILE_FLAGS ${BISON_FLAGS})

    include_directories (${CMAKE_CURRENT_BINARY_DIR})
    include_directories (${CMAKE_CURRENT_SOURCE_DIR})
    add_library(afmongodb MODULE ${AFMONGODB_SOURCES})

    target_link_libraries(afmongodb ${LIBRABBITMQ_LIBRARIES})
    target_include_directories(afmongodb PUBLIC ${LIBMONGOC_INCLUDE_DIRS})
    target_compile_options(afmongodb PUBLIC ${LIBMONGOC_CFLAGS_OTHER})

    add_dependencies(afmongodb syslog-ng)
    
    install(TARGETS afmongodb LIBRARY DESTINATION lib/syslog-ng/)

endif(LIBMONGOC_FOUND)
