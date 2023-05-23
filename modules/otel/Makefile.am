OTEL_PROTO_SRCDIR = $(top_srcdir)/modules/otel/opentelemetry-proto
OTEL_PROTO_BUILDDIR = $(top_builddir)/modules/otel/proto

if ENABLE_OTEL

OTEL_PROTO_PROTOC_FLAGS = $(PROTOC_GEN_GRPC_CPP_PLUGIN_FLAGS) --proto_path=$(OTEL_PROTO_SRCDIR) --cpp_out=$(OTEL_PROTO_BUILDDIR)

modules/otel/proto/%.grpc.pb.cc modules/otel/proto/%.grpc.pb.h:
	$(PROTOC) $(OTEL_PROTO_PROTOC_FLAGS) --grpc-cpp_out=$(OTEL_PROTO_BUILDDIR) $*.proto

modules/otel/proto/%.pb.cc modules/otel/proto/%.pb.h:
	$(PROTOC) $(OTEL_PROTO_PROTOC_FLAGS) $*.proto

OTEL_PROTO_BUILT_SOURCES = \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/common/v1/common.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/common/v1/common.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/resource/v1/resource.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/resource/v1/resource.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/logs/v1/logs.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/logs/v1/logs.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/metrics/v1/metrics.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/metrics/v1/metrics.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/trace/v1/trace.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/trace/v1/trace.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/logs/v1/logs_service.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/logs/v1/logs_service.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.cc \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/trace/v1/trace_service.pb.h \
  $(OTEL_PROTO_BUILDDIR)/opentelemetry/proto/collector/trace/v1/trace_service.pb.cc

noinst_LTLIBRARIES += modules/otel/libotel_cpp.la

nodist_modules_otel_libotel_cpp_la_SOURCES = $(OTEL_PROTO_BUILT_SOURCES)

modules_otel_libotel_cpp_la_SOURCES = \
  modules/otel/otel-source.h \
  modules/otel/otel-source.hpp \
  modules/otel/otel-source.cpp \
  modules/otel/otel-source-services.hpp \
  modules/otel/otel-source-services.cpp

modules_otel_libotel_cpp_la_CXXFLAGS = \
  $(AM_CXXFLAGS) \
  $(PROTOBUF_CLFAGS) \
  $(GRPCPP_CFLAGS) \
  -I$(OTEL_PROTO_BUILDDIR) \
  -I$(top_srcdir)/modules/otel \
  -I$(top_builddir)/modules/otel

modules_otel_libotel_cpp_la_LIBADD = $(MODULE_DEPS_LIBS) $(PROTOBUF_LIBS) $(GRPCPP_LIBS)
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

CLEANFILES += \
  $(OTEL_PROTO_BUILT_SOURCES)

BUILT_SOURCES += \
  modules/otel/otel-grammar.y \
  modules/otel/otel-grammar.c \
  modules/otel/otel-grammar.h \
  $(OTEL_PROTO_BUILT_SOURCES)

EXTRA_DIST += \
  modules/otel/otel-grammar.ym \
  modules/otel/CMakeLists.txt \
  $(OTEL_PROTO_SRCDIR)

.PHONY: modules/otel/ mod-otel
