include_directories(${CRITERION_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/libtest)
link_libraries(${CRITERION_LIBRARIES} syslog-ng-test syslog-ng)

set(TEST_COMMAND_LINE_FLAGS --jobs 1)

add_executable(test_clone_logmsg test_clone_logmsg.c)
add_test(NAME test_clone_logmsg
         COMMAND test_clone_logmsg TEST_COMMAND_LINE_FLAGS)

add_executable(test_dnscache test_dnscache.c)
add_test(NAME test_dnscache
         COMMAND test_dnscache TEST_COMMAND_LINE_FLAGS)

add_executable(test_findcrlf test_findcrlf.c)
add_test(NAME test_findcrlf
         COMMAND test_findcrlf TEST_COMMAND_LINE_FLAGS)

add_executable(test_hostid test_hostid.c)
add_test(NAME test_hostid
         COMMAND test_hostid TEST_COMMAND_LINE_FLAGS)

add_executable(test_logqueue test_logqueue.c)
add_test(NAME test_logqueue
         COMMAND test_logqueue TEST_COMMAND_LINE_FLAGS)

add_executable(test_logwriter test_logwriter.c)
add_test(NAME test_logwriter
         COMMAND test_logwriter TEST_COMMAND_LINE_FLAGS)

add_executable(test_matcher test_matcher.c)
add_test(NAME test_matcher
         COMMAND test_matcher TEST_COMMAND_LINE_FLAGS)

add_executable(test_msgparse test_msgparse.c)
add_test(NAME test_msgparse
         COMMAND test_msgparse TEST_COMMAND_LINE_FLAGS)

add_executable(test_msgsdata test_msgsdata.c)
add_test(NAME test_msgsdata
         COMMAND test_msgsdata TEST_COMMAND_LINE_FLAGS)

add_executable(test_nvtable test_nvtable.c)
add_test(NAME test_nvtable
         COMMAND test_nvtable TEST_COMMAND_LINE_FLAGS)

add_executable(test_persist_state test_persist_state.c)
add_test(NAME test_persist_state
         COMMAND test_persist_state TEST_COMMAND_LINE_FLAGS)

add_executable(test_ringbuffer test_ringbuffer.c)
add_test(NAME test_ringbuffer
         COMMAND test_ringbuffer TEST_COMMAND_LINE_FLAGS)

add_executable(test_serialize test_serialize.c)
add_test(NAME test_serialize
         COMMAND test_serialize TEST_COMMAND_LINE_FLAGS)

add_executable(test_tags test_tags.c)
add_test(NAME test_tags
         COMMAND test_tags TEST_COMMAND_LINE_FLAGS)

add_executable(test_value_pairs test_value_pairs.c)
add_test(NAME test_value_pairs
         COMMAND test_value_pairs TEST_COMMAND_LINE_FLAGS)

add_executable(test_value_pairs_walk test_value_pairs_walk.c)
add_test(NAME test_value_pairs_walk
         COMMAND test_value_pairs_walk TEST_COMMAND_LINE_FLAGS)

add_executable(test_zone test_zone.c)
add_test(NAME test_zone
         COMMAND test_zone TEST_COMMAND_LINE_FLAGS)
