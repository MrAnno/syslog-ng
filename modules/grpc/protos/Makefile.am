GRPC_PROTOS_SRCDIR = $(top_srcdir)/modules/grpc/protos
GRPC_PROTOS_BUILDDIR = $(top_builddir)/modules/grpc/protos

if ENABLE_GRPC

GOOGLEAPIS_PROTO_SRCDIR = $(GRPC_PROTOS_SRCDIR)/googleapis-proto
GOOGLEAPIS_PROTO_BUILDDIR = $(GRPC_PROTOS_BUILDDIR)/googleapis-proto

OPENTELEMETRY_PROTO_SRCDIR = $(GRPC_PROTOS_SRCDIR)/opentelemetry-proto
OPENTELEMETRY_PROTO_BUILDDIR = $(GRPC_PROTOS_BUILDDIR)/opentelemetry-proto

OPENTELEMETRY_PROTO_PROTOC_FLAGS = $(PROTOC_GEN_GRPC_CPP_PLUGIN_FLAGS) --proto_path=$(OPENTELEMETRY_PROTO_SRCDIR) --cpp_out=$(OPENTELEMETRY_PROTO_BUILDDIR)
GOOGLEAPIS_PROTO_PROTOC_FLAGS = $(PROTOC_GEN_GRPC_CPP_PLUGIN_FLAGS) --proto_path=$(GOOGLEAPIS_PROTO_SRCDIR) --cpp_out=$(GOOGLEAPIS_PROTO_BUILDDIR)

modules/grpc/protos/googleapis-proto/%.grpc.pb.cc modules/grpc/protos/googleapis-proto/%.grpc.pb.h:
	mkdir -p $(GOOGLEAPIS_PROTO_BUILDDIR)
	$(PROTOC) $(GOOGLEAPIS_PROTO_PROTOC_FLAGS) --grpc-cpp_out=$(GOOGLEAPIS_PROTO_BUILDDIR) $*.proto

modules/grpc/protos/googleapis-proto/%.pb.cc modules/grpc/protos/googleapis-proto/%.pb.h:
	mkdir -p $(GOOGLEAPIS_PROTO_BUILDDIR)
	$(PROTOC) $(GOOGLEAPIS_PROTO_PROTOC_FLAGS) $*.proto

modules/grpc/protos/opentelemetry-proto/%.grpc.pb.cc modules/grpc/protos/opentelemetry-proto/%.grpc.pb.h:
	mkdir -p $(OPENTELEMETRY_PROTO_BUILDDIR)
	$(PROTOC) $(OPENTELEMETRY_PROTO_PROTOC_FLAGS) --grpc-cpp_out=$(OPENTELEMETRY_PROTO_BUILDDIR) $*.proto

modules/grpc/protos/opentelemetry-proto/%.pb.cc modules/grpc/protos/opentelemetry-proto/%.pb.h:
	mkdir -p $(OPENTELEMETRY_PROTO_BUILDDIR)
	$(PROTOC) $(OPENTELEMETRY_PROTO_PROTOC_FLAGS) $*.proto

GRPC_PROTOS_BUILT_SOURCES = \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/annotations.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/annotations.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/arrow.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/arrow.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/avro.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/avro.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/protobuf.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/protobuf.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/storage.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/storage.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/stream.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/stream.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/table.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/table.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/field_behavior.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/field_behavior.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/resource.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/resource.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/annotations.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/annotations.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/http.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/http.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/client.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/client.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/launch_stage.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/launch_stage.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/rpc/status.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/rpc/status.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/annotations.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/annotations.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/arrow.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/arrow.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/avro.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/avro.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/protobuf.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/protobuf.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/storage.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/storage.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/stream.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/stream.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/table.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/cloud/bigquery/storage/v1/table.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/field_behavior.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/field_behavior.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/resource.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/resource.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/annotations.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/annotations.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/http.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/http.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/client.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/client.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/launch_stage.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/api/launch_stage.grpc.pb.cc \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/rpc/status.grpc.pb.h \
  $(GRPC_PROTOS_BUILDDIR)/googleapis-proto/google/rpc/status.grpc.pb.cc \
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
  -I$(GOOGLEAPIS_PROTO_SRCDIR) \
  -I$(GOOGLEAPIS_PROTO_BUILDDIR) \
  -I$(OPENTELEMETRY_PROTO_SRCDIR) \
  -I$(OPENTELEMETRY_PROTO_BUILDDIR)

if ENABLE_EXTRA_WARNINGS
modules_grpc_protos_libgrpc_protos_la_CXXFLAGS += -Wno-switch-default
endif

modules_grpc_protos_libgrpc_protos_la_LIBADD = $(MODULE_DEPS_LIBS) $(PROTOBUF_LIBS) $(GRPCPP_LIBS) -lstdc++
modules_grpc_protos_libgrpc_protos_la_DEPENDENCIES = $(MODULE_DEPS_LIBS)

endif

CLEANFILES += \
  $(GRPC_PROTOS_BUILT_SOURCES)

BUILT_SOURCES += \
  $(GRPC_PROTOS_BUILT_SOURCES)

EXTRA_DIST += \
  $(GRPC_PROTOS_SRCDIR)
