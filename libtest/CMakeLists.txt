find_package(Criterion)
if (CRITERION_FOUND)

set(LIBTEST_SOURCES
	cmsg_parse_lib.c
	cmsg_parse_lib.h
	queue_utils_lib.c
	queue_utils_lib.h
)

add_library(syslog-ng-test ${LIBTEST_SOURCES})
target_link_libraries(syslog-ng-test PUBLIC ${CRITERION_LIBRARIES})
target_include_directories(syslog-ng-test PRIVATE ${CRITERION_INCLUDE_DIRS})

endif()
