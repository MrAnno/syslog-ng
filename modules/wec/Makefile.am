modules_wec_libwec_la_SOURCES	=	\
	modules/wec/wec-source.c		\
	modules/wec/wec-source.h		\
	modules/wec/socket-options.c        	\
	modules/wec/socket-options.h		\
	modules/wec/transport-mapper.c		\
	modules/wec/transport-mapper.h		\
	modules/wec/afinet.c			\
	modules/wec/afinet.h			\
	modules/wec/afinet-source.c		\
	modules/wec/afinet-source.h		\
	modules/wec/socket-options-inet.c		\
	modules/wec/socket-options-inet.h   	\
	modules/wec/transport-mapper-inet.c 	\
	modules/wec/transport-mapper-inet.h 	\
	modules/wec/wec-grammar.y		\
	modules/wec/wec-parser.c		\
	modules/wec/wec-parser.h		\
	modules/wec/wec-plugin.c


module_LTLIBRARIES				+= modules/wec/libwec.la
modules_wec_libwec_la_CPPFLAGS	=	\
	$(AM_CPPFLAGS)	\
	-I${top_srcdir}/modules/wec		\
	-I${top_builddir}/modules/wec

modules_wec_libwec_la_LIBADD	=	\
	$(MODULE_DEPS_LIBS) $(OPENSSL_LIBS)	\
	$(ZLIB_LIBS)
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
