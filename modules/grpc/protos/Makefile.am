GRPC_PROTOS_SRCDIR = $(top_srcdir)/modules/grpc/protos
GRPC_PROTOS_BUILDDIR = $(top_builddir)/modules/grpc/protos

if ENABLE_GRPC

OPENTELEMETRY_PROTO_SRCDIR = $(GRPC_PROTOS_SRCDIR)/opentelemetry-proto
OPENTELEMETRY_PROTO_BUILDDIR = $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto

OPENTELEMETRY_PROTO_PROTOC_FLAGS = $(PROTOC_GEN_GRPC_CPP_PLUGIN_FLAGS) --proto_path=$(OPENTELEMETRY_PROTO_SRCDIR) --cpp_out=$(OPENTELEMETRY_PROTO_BUILDDIR)

modules/grpc/protos/opentelemetry-proto/%.grpc.pb.cc modules/grpc/protos/opentelemetry-proto/%.grpc.pb.h:
	mkdir -p $(OPENTELEMETRY_PROTO_BUILDDIR)
	$(PROTOC) $(OPENTELEMETRY_PROTO_PROTOC_FLAGS) --grpc-cpp_out=$(OPENTELEMETRY_PROTO_BUILDDIR) $*.proto

modules/grpc/protos/opentelemetry-proto/%.pb.cc modules/grpc/protos/opentelemetry-proto/%.pb.h:
	mkdir -p $(OPENTELEMETRY_PROTO_BUILDDIR)
	$(PROTOC) $(OPENTELEMETRY_PROTO_PROTOC_FLAGS) $*.proto

GRPC_PROTOS_BUILT_SOURCES = \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/common/v1/common.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/common/v1/common.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/resource/v1/resource.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/resource/v1/resource.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/logs/v1/logs.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/logs/v1/logs.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/metrics/v1/metrics.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/metrics/v1/metrics.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/trace/v1/trace.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/trace/v1/trace.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/logs/v1/logs_service.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/logs/v1/logs_service.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/trace/v1/trace_service.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto/opentelemetry/proto/collector/trace/v1/trace_service.pb.cc

lib_LTLIBRARIES += modules/grpc/protos/libgrpc-protos.la

nodist_modules_grpc_protos_libgrpc_protos_la_SOURCES = $(GRPC_PROTOS_BUILT_SOURCES)

modules_grpc_protos_libgrpc_protos_la_CXXFLAGS = \
  $(AM_CXXFLAGS) \
  $(PROTOBUF_CLFAGS) \
  $(GRPCPP_CFLAGS) \
  -I$(OPENTELEMETRY_PROTO_SRCDIR) \
  -I$(OPENTELEMETRY_PROTO_BUILDDIR)

modules_grpc_protos_libgrpc_protos_la_LIBADD = $(MODULE_DEPS_LIBS) $(PROTOBUF_LIBS) $(GRPCPP_LIBS) -lstdc++
modules_grpc_protos_libgrpc_protos_la_DEPENDENCIES = $(MODULE_DEPS_LIBS)

endif

CLEANFILES += \
  $(GRPC_PROTOS_BUILT_SOURCES)

BUILT_SOURCES += \
  $(GRPC_PROTOS_BUILT_SOURCES)

EXTRA_DIST += \
  $(GRPC_PROTOS_SRCDIR)
