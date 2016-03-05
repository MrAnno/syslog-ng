set(AFSQL_SOURCES
    afsql.c
    afsql.h
    afsql-parser.c
    afsql-parser.h
    afsql-plugin.c
    ${CMAKE_CURRENT_BINARY_DIR}/afsql-grammar.h
    ${CMAKE_CURRENT_BINARY_DIR}/afsql-grammar.c
)

option(ENABLE_SQL "Enable SQL plugin" ON)

find_package(LibDBI)
find_package(OpenSSL)

if(LIBDBI_FOUND AND ENABLE_SQL)
    set(BUILD_AFSQL ON)
else()
    set(BUILD_AFSQL OFF)
endif()

if (BUILD_AFSQL)
    generate_y_from_ym(modules/afsql/afsql-grammar)

    bison_target(SqlGrammar
        ${CMAKE_CURRENT_BINARY_DIR}/afsql-grammar.y
        ${CMAKE_CURRENT_BINARY_DIR}/afsql-grammar.c
        COMPILE_FLAGS ${BISON_FLAGS})

    add_library(afsql MODULE ${AFSQL_SOURCES})
    target_link_libraries (afsql PUBLIC ${LIBDBI_LIBRARIES} PUBLIC ${OPENSSL_LIBRARIES})
    target_include_directories (afsql SYSTEM PRIVATE ${LIBDBI_INCLUDE_DIRS})
    target_include_directories (afsql PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(afsql PRIVATE syslog-ng)
    
    install(TARGETS afsql LIBRARY DESTINATION lib/syslog-ng/)
endif()
