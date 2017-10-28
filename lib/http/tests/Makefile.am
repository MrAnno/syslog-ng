if ENABLE_CRITERION

lib_http_tests_TESTS = \
  lib/http/tests/test_http-parser \
  lib/http/tests/test_http-message

check_PROGRAMS += ${lib_http_tests_TESTS}

lib_http_tests_test_http_parser_CFLAGS = $(TEST_CFLAGS)
lib_http_tests_test_http_parser_LDADD = $(TEST_LDADD)

lib_http_tests_test_http_message_CFLAGS = $(TEST_CFLAGS)
lib_http_tests_test_http_message_LDADD = $(TEST_LDADD)

endif
