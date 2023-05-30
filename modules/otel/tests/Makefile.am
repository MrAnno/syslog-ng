if ENABLE_OTEL

modules_otel_tests_TESTS = \
  modules/otel/tests/test_otel_protobuf_parser

check_PROGRAMS += ${modules_otel_tests_TESTS}

modules_otel_tests_test_otel_protobuf_parser_SOURCES = \
  modules/otel/tests/test-otel-protobuf-parser.cpp

modules_otel_tests_test_otel_protobuf_parser_DEPENDENCIES = \
  $(top_builddir)/modules/otel/libotel_cpp.la

modules_otel_tests_test_otel_protobuf_parser_CXXFLAGS = \
  $(TEST_CXXFLAGS) \
  -I$(OTEL_PROTO_BUILDDIR) \
  -I$(top_srcdir)/modules/otel \
  -I$(top_builddir)/modules/otel

modules_otel_tests_test_otel_protobuf_parser_LDADD = \
  $(TEST_LDADD) \
  $(top_builddir)/modules/otel/libotel_cpp.la

endif

EXTRA_DIST += \
    modules/otel/tests/CMakeLists.txt
