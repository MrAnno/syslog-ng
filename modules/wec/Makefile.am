modules_wec_libwec_la_SOURCES	=	\
	modules/wec/wec.c			\
	modules/wec/wec.h			\
	modules/wec/wec-source.c		\
	modules/wec/wec-source.h		\
	modules/wec/wec-dest.c		\
	modules/wec/wec-dest.h		\
	modules/wec/socket-options.c        	\
	modules/wec/socket-options.h		\
	modules/wec/transport-mapper.c		\
	modules/wec/transport-mapper.h		\
	modules/wec/afinet.c			\
	modules/wec/afinet.h			\
	modules/wec/afinet-source.c		\
	modules/wec/afinet-source.h		\
	modules/wec/afinet-dest.c			\
	modules/wec/afinet-dest.h			\
	modules/wec/socket-options-inet.c		\
	modules/wec/socket-options-inet.h   	\
	modules/wec/transport-mapper-inet.c 	\
	modules/wec/transport-mapper-inet.h 	\
	modules/wec/afunix-source.c		\
	modules/wec/afunix-source.h		\
	modules/wec/afunix-dest.c			\
	modules/wec/afunix-dest.h			\
	modules/wec/transport-mapper-unix.c 	\
	modules/wec/transport-mapper-unix.h 	\
	modules/wec/transport-unix-socket.c	\
	modules/wec/transport-unix-socket.h	\
	modules/wec/unix-credentials.c		\
	modules/wec/unix-credentials.h		\
	modules/wec/wec-grammar.y		\
	modules/wec/wec-parser.c		\
	modules/wec/wec-parser.h		\
	modules/wec/wec-plugin.c    \
	modules/wec/systemd-syslog-source.h       \
	modules/wec/systemd-syslog-source.c	\
	modules/wec/wec-systemd-override.h

module_LTLIBRARIES				+= modules/wec/libwec.la
modules_wec_libwec_la_CPPFLAGS	=	\
	$(AM_CPPFLAGS) $(libsystemd_CFLAGS)	\
	-I${top_srcdir}/modules/wec		\
	-I${top_builddir}/modules/wec

modules_wec_libwec_la_LIBADD	=	\
	$(MODULE_DEPS_LIBS) $(OPENSSL_LIBS)	\
	$(ZLIB_LIBS) $(LIBNET_LIBS)	\
	$(LIBWRAP_LIBS) $(libsystemd_LIBS)
modules_wec_libwec_la_DEPENDENCIES= 	\
	$(MODULE_DEPS_LIBS)
modules_wec_libwec_la_LDFLAGS	= 	\
	$(MODULE_LDFLAGS)

modules/wec modules/wec/ mod-wec: \
	modules/wec/libwec.la

BUILT_SOURCES					+=	\
	modules/wec/wec-grammar.y		\
	modules/wec/wec-grammar.c		\
	modules/wec/wec-grammar.h
EXTRA_DIST 					+=	\
	modules/wec/wec-grammar.ym

CLEANFILES					+= \
	modules/wec/libwec.la

.PHONY: modules/wec/ mod-wec

include modules/wec/tests/Makefile.am
