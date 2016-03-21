include_directories(${CRITERION_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/libtest)
link_libraries(${CRITERION_LIBRARIES} syslog-ng-test syslog-ng)

add_executable(test_findcrlf ctest_findcrlf.c)
add_test(test_findcrlf test_findcrlf --jobs 1)

add_executable(test_matcher ctest_matcher.c)
add_test(test_matcher test_matcher --jobs 1)

add_executable(test_dnscache ctest_dnscache.c)
add_test(test_dnscache test_dnscache --jobs 1)

add_executable(test_serialize ctest_serialize.c)
add_test(test_serialize test_serialize --jobs 1)

add_executable(test_ringbuffer ctest_ringbuffer.c)
add_test(test_ringbuffer test_ringbuffer --jobs 1)

add_executable(test_tags ctest_tags.c)
add_test(test_tags test_tags --jobs 1)

add_executable(test_zone ctest_zone.c)
add_test(test_zone test_zone --jobs 1)

add_executable(test_hostid ctest_hostid.c)
add_test(test_hostid test_hostid --jobs 1)

add_executable(test_value_pairs ctest_value_pairs.c)
add_test(test_value_pairs test_value_pairs --jobs 1)

add_executable(test_value_pairs_walk ctest_value_pairs_walk.c)
add_test(test_value_pairs_walk test_value_pairs_walk --jobs 1)

add_executable(test_logwriter ctest_logwriter.c)
add_test(test_logwriter test_logwriter --jobs 1)

add_executable(test_nvtable ctest_nvtable.c)
add_test(test_nvtable test_nvtable --jobs 1)

add_executable(test_msgsdata ctest_msgsdata.c)
add_test(test_msgsdata test_msgsdata --jobs 1)

add_executable(test_logqueue ctest_logqueue.c)
add_test(test_logqueue test_logqueue --jobs 1)
