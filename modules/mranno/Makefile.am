module_LTLIBRARIES      += modules/mranno/libmranno.la
modules_mranno_libmranno_la_SOURCES = \
  modules/mranno/mranno-grammar.y       \
  modules/mranno/mranno-parser.c        \
  modules/mranno/mranno-parser.h        \
  modules/mranno/mranno-plugin.c        \
  modules/mranno/mranno.c               \
  modules/mranno/mranno.h               \
  modules/mranno/mranno-source.c        \
  modules/mranno/mranno-source.h        \
  modules/mranno/mranno-source-options.h

BUILT_SOURCES       +=      \
  modules/mranno/mranno-grammar.y       \
  modules/mranno/mranno-grammar.c       \
  modules/mranno/mranno-grammar.h

EXTRA_DIST        +=      \
  modules/mranno/mranno-grammar.ym

modules_mranno_libmranno_la_CPPFLAGS  =     \
  $(AM_CPPFLAGS)            \
  -I$(top_srcdir)/modules/mranno        \
  -I$(top_builddir)/modules/mranno
modules_mranno_libmranno_la_LIBADD  = $(MODULE_DEPS_LIBS)
modules_mranno_libmranno_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_mranno_libmranno_la_DEPENDENCIES= $(MODULE_DEPS_LIBS)

modules/mranno modules/mranno/ mod-mranno: modules/mranno/libmranno.la
.PHONY: modules/mranno/ mod-mranno

