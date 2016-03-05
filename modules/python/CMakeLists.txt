option(ENABLE_PYTHON "Enable Python module" ON)

find_package(PythonLibs)

set(PYTHON_SOURCES
    python-module.h
    python-config.h
    python-config.c
    python-helpers.h
    python-helpers.c
    python-main.h
    python-main.c
    python-plugin.c
    python-value-pairs.c
    python-value-pairs.h
    python-dest.c
    python-dest.h
    python-tf.c
    python-tf.h
    python-parser.c
    python-parser.h
    python-logmsg.h
    python-logmsg.c
    python-debugger.c
    python-debugger.h
    ${CMAKE_CURRENT_BINARY_DIR}/python-grammar.c
    ${CMAKE_CURRENT_BINARY_DIR}/python-grammar.h
)

generate_y_from_ym(modules/python/python-grammar)

bison_target(PythonGrammar
    ${CMAKE_CURRENT_BINARY_DIR}/python-grammar.y
    ${CMAKE_CURRENT_BINARY_DIR}/python-grammar.c
    COMPILE_FLAGS ${BISON_FLAGS})

if (ENABLE_PYTHON AND PYTHONLIBS_FOUND)
    set(BUILD_PYTHON TRUE)
else()
    set(BUILD_PYTHON FALSE)
endif()

if (BUILD_PYTHON)
    add_library(python MODULE ${PYTHON_SOURCES})
    target_include_directories (python PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories (python PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories (python PRIVATE ${PYTHON_INCLUDE_DIRS} ${CORE_INCLUDE_DIRS})
    target_link_libraries(python PRIVATE syslog-ng ${PYTHON_LIBRARIES} ${Eventlog_LIBRARIES} ${Ivykis_LIBRARIES})
endif()
