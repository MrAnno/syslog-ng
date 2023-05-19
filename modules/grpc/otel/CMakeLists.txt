set(OTEL_DEPS_FOUND ${ENABLE_CPP})

if (OTEL_DEPS_FOUND)
  find_package(Protobuf 3.6.1)
  if (NOT Protobuf_FOUND)
    set(OTEL_DEPS_FOUND FALSE)
  endif()
endif()

if (OTEL_DEPS_FOUND)
  find_package(gRPC 1.16.1)
  if (NOT gRPC_FOUND)
    set(OTEL_DEPS_FOUND FALSE)
  endif()
endif()

module_switch(ENABLE_OTEL "Enable OpenTelemetry" OTEL_DEPS_FOUND)

if (NOT ENABLE_OTEL)
  return()
endif()

include_directories(${Protobuf_INCLUDE_DIRS})

set(OTEL_CPP_SOURCES
  otel-source.cpp
  otel-source.hpp
  otel-source.h
)

set(OTEL_SOURCES
  otel-plugin.c
  otel-parser.c
  otel-parser.h
)

add_module(
  TARGET otel-cpp
  SOURCES ${OTEL_CPP_SOURCES}
  LIBRARY_TYPE STATIC
)

add_module(
  TARGET otel
  GRAMMAR otel-grammar
  DEPENDS otel-cpp
  SOURCES ${OTEL_SOURCES}
)
