set (BASICFUNCS_SOURCES
  basic-funcs.c
)

add_library(basicfuncs MODULE ${BASICFUNCS_SOURCES})
target_include_directories (basicfuncs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(basicfuncs PRIVATE syslog-ng)

# NOTE: check EXTRA_DIST in Makefile.am for installation

install(TARGETS basicfuncs LIBRARY DESTINATION lib/syslog-ng/)
