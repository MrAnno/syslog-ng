noinst_LTLIBRARIES += modules/examples/sources/libhttp-source.la

modules_examples_sources_libhttp_source_la_SOURCES = \
  modules/examples/sources/http/http-source-grammar.y \
  modules/examples/sources/http/http-source-parser.c \
  modules/examples/sources/http/http-source-parser.h

BUILT_SOURCES += \
  modules/examples/sources/http/http-source-grammar.y \
  modules/examples/sources/http/http-source-grammar.c \
  modules/examples/sources/http/http-source-grammar.h

EXTRA_DIST += \
  modules/examples/sources/http/http-source-grammar.ym

modules_examples_sources_libhttp_source_la_CPPFLAGS  = \
  $(AM_CPPFLAGS) \
  -I$(top_srcdir)/modules/examples/sources/http \
  -I$(top_builddir)/modules/examples/sources/http

modules_examples_sources_libhttp_source_la_LIBADD  = $(MODULE_DEPS_LIBS)
modules_examples_sources_libhttp_source_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_examples_sources_libhttp_source_la_DEPENDENCIES = $(MODULE_DEPS_LIBS)
