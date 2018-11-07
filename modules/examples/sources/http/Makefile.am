noinst_LTLIBRARIES += modules/examples/sources/libhttp-source.la

modules_examples_sources_libhttp_source_la_SOURCES = \
  modules/examples/sources/http/http-source-grammar.y \
  modules/examples/sources/http/http-source-parser.c \
  modules/examples/sources/http/http-source-parser.h \
  modules/examples/sources/http/http-source.h \
  modules/examples/sources/http/http-source.c

BUILT_SOURCES += \
  modules/examples/sources/http/http-source-grammar.y \
  modules/examples/sources/http/http-source-grammar.c \
  modules/examples/sources/http/http-source-grammar.h

EXTRA_DIST += \
  modules/examples/sources/http/http-source-grammar.ym

modules_examples_sources_libhttp_source_la_CPPFLAGS  = \
  $(AM_CPPFLAGS) \
  $(JSON_CFLAGS) \
  -I$(top_srcdir)/modules/examples/sources/http \
  -I$(top_builddir)/modules/examples/sources/http

modules_examples_sources_libhttp_source_la_LIBADD  = $(MODULE_DEPS_LIBS) $(JSON_LIBS)
modules_examples_sources_libhttp_source_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_examples_sources_libhttp_source_la_DEPENDENCIES = $(MODULE_DEPS_LIBS) $(JSON_DEPENDENCY)
