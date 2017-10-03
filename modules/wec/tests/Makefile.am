TEST_TRANSPORT_MAPPER_CFLAGS = $(TEST_CFLAGS)

TRANSPORT_MAPPER_LIB = 					\
	modules/wec/tests/transport-mapper-lib.c	\
	modules/wec/tests/transport-mapper-lib.h

modules_wec_tests_TESTS			=		\
	modules/wec/tests/test-transport-mapper		\
	modules/wec/tests/test-transport-mapper-inet	\
	modules/wec/tests/test-transport-mapper-unix

check_PROGRAMS					+=	\
	$(modules_wec_tests_TESTS)

modules_wec_tests_test_transport_mapper_CFLAGS = 	\
	$(TEST_TRANSPORT_MAPPER_CFLAGS)			\
	-I$(top_srcdir)/modules/wec

modules_wec_tests_test_transport_mapper_LDADD = 	\
	$(TEST_LDADD)

modules_wec_tests_test_transport_mapper_LDFLAGS =	\
	-dlpreopen $(top_builddir)/modules/wec/libwec.la

modules_wec_tests_test_transport_mapper_SOURCES = 	\
	modules/wec/tests/test-transport-mapper.c	\
	$(TRANSPORT_MAPPER_LIB)

modules_wec_tests_test_transport_mapper_inet_CFLAGS = 	\
	$(TEST_TRANSPORT_MAPPER_CFLAGS)				\
	-I$(top_srcdir)/modules/wec

modules_wec_tests_test_transport_mapper_inet_LDADD = 	\
	$(TEST_LDADD)

modules_wec_tests_test_transport_mapper_inet_LDFLAGS =	\
	-dlpreopen $(top_builddir)/modules/wec/libwec.la

modules_wec_tests_test_transport_mapper_inet_SOURCES = 	\
	modules/wec/tests/test-transport-mapper-inet.c	\
	$(TRANSPORT_MAPPER_LIB)

modules_wec_tests_test_transport_mapper_unix_CFLAGS = 	\
	$(TEST_TRANSPORT_MAPPER_CFLAGS)				\
	-I$(top_srcdir)/modules/wec

modules_wec_tests_test_transport_mapper_unix_LDADD = 	\
	$(TEST_LDADD)

modules_wec_tests_test_transport_mapper_unix_LDFLAGS =	\
	-dlpreopen $(top_builddir)/modules/wec/libwec.la

modules_wec_tests_test_transport_mapper_unix_SOURCES = 	\
	modules/wec/tests/test-transport-mapper-unix.c	\
	$(TRANSPORT_MAPPER_LIB)
