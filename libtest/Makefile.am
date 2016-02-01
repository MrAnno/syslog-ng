libtestdir			    =	\
	$(pkglibdir)/libtest
libtestincludedir		    =	\
	$(pkgincludedir)/libtest

libtest_LIBRARIES		    =	\
	libtest/libsyslog-ng-test.a

libtest_libsyslog_ng_test_a_SOURCES =   \
	libtest/libtest.c		\
	libtest/testutils.c		\
	libtest/testutils.h		\
	libtest/msg_parse_lib.c		\
	libtest/msg_parse_lib.h		\
	libtest/template_lib.c		\
	libtest/template_lib.h		\
	libtest/proto_lib.c		\
	libtest/proto_lib.h		\
	libtest/persist_lib.h		\
	libtest/persist_lib.c		\
	libtest/mock-transport.c	\
	libtest/mock-transport.h    \
	libtest/config_parse_lib.c  \
	libtest/config_parse_lib.h  \
	libtest/Unity/src/unity.c   \
	libtest/Unity/src/unity.h   \
	libtest/Unity/extras/fixture/src/unity_fixture.c \
	libtest/Unity/extras/fixture/src/unity_fixture.h

libtest_libsyslog_ng_test_a_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/libtest/Unity/src

libtestinclude_HEADERS		    =	\
	libtest/testutils.h		\
	libtest/msg_parse_lib.h		\
	libtest/template_lib.h		\
	libtest/proto_lib.h		\
	libtest/persist_lib.h		\
	libtest/mock-transport.h    \
	libtest/Unity/src/unity.h   \
	libtest/Unity/extras/fixture/src/unity_fixture.h

pkgconfig_DATA			   +=	\
	libtest/syslog-ng-test.pc
