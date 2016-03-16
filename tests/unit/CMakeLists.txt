include_directories(${CRITERION_INCLUDE_DIRS})
link_libraries(${CRITERION_LIBRARIES})
link_libraries(syslog-ng)

add_executable(test_findcrlf ctest_findcrlf.c)
add_test(test_findcrlf test_findcrlf)

add_executable(test_matcher ctest_matcher.c)
add_test(test_matcher test_matcher)

add_executable(test_dnscache ctest_dnscache.c)
add_test(test_dnscache test_dnscache)
