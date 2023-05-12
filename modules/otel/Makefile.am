if ENABLE_OTEL

noinst_LTLIBRARIES += modules/otel/libotel_cpp.la

modules_otel_libotel_cpp_la_SOURCES = \
  modules/otel/otel-source.h \
  modules/otel/otel-source.hpp \
  modules/otel/otel-source.cpp

modules_otel_libotel_cpp_la_CXXFLAGS = \
  $(AM_CXXFLAGS) \
  -I$(top_srcdir)/modules/otel \
  -I$(top_builddir)/modules/otel

modules_otel_libotel_cpp_la_LIBADD = $(MODULE_DEPS_LIBS)
modules_otel_libotel_cpp_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_otel_libotel_cpp_la_DEPENDENCIES = $(MODULE_DEPS_LIBS)

module_LTLIBRARIES += modules/otel/libotel.la

modules_otel_libotel_la_SOURCES = \
  modules/otel/otel-plugin.c \
  modules/otel/otel-grammar.y \
  modules/otel/otel-parser.c \
  modules/otel/otel-parser.h

modules_otel_libotel_la_CPPFLAGS = \
  $(AM_CPPFLAGS) \
  -I$(top_srcdir)/modules/otel \
  -I$(top_builddir)/modules/otel

modules_otel_libotel_la_LIBADD = \
  $(MODULE_DEPS_LIBS) \
  $(top_builddir)/modules/otel/libotel_cpp.la \
  -lstdc++

modules_otel_libotel_la_LDFLAGS = $(MODULE_LDFLAGS)

modules_otel_libotel_la_DEPENDENCIES = \
  $(MODULE_DEPS_LIBS) \
  $(top_builddir)/modules/otel/libotel_cpp.la

modules/otel modules/otel/ mod-otel: modules/otel/libotel.la

endif

BUILT_SOURCES += \
  modules/otel/otel-grammar.y \
  modules/otel/otel-grammar.c \
  modules/otel/otel-grammar.h

EXTRA_DIST += \
  modules/otel/otel-grammar.ym \
  modules/otel/CMakeLists.txt

.PHONY: modules/otel/ mod-otel
