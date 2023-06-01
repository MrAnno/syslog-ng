/*
 * Copyright (c) 2023 Attila Szakacs
 * Copyright (c) 2023 László Várady
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#ifndef OTEL_SOURCE_SERVICES_HPP
#define OTEL_SOURCE_SERVICES_HPP

#include "opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h"
#include "opentelemetry/proto/collector/logs/v1/logs_service.grpc.pb.h"
#include "opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.h"

#include "otel-source.hpp"

using namespace opentelemetry::proto;
using namespace collector::trace::v1;
using namespace collector::logs::v1;
using namespace collector::metrics::v1;

class OtelSourceTraceService final : public TraceService::Service
{
public:
  OtelSourceTraceService(OtelSourceDriverCpp &driver_) : driver(driver_) {};
  grpc::Status Export(grpc::ServerContext *context, const ExportTraceServiceRequest *request,
                      ExportTraceServiceResponse *response) override;

private:
  OtelSourceDriverCpp &driver;
};

class OtelSourceLogsService final : public LogsService::Service
{
public:
  OtelSourceLogsService(OtelSourceDriverCpp &driver_) : driver(driver_) {};
  grpc::Status Export(grpc::ServerContext *context, const ExportLogsServiceRequest *request,
                      ExportLogsServiceResponse *response) override;

private:
  OtelSourceDriverCpp &driver;
};

class OtelSourceMetricsService final : public MetricsService::Service
{
public:
  OtelSourceMetricsService(OtelSourceDriverCpp &driver_) : driver(driver_) {};
  grpc::Status Export(grpc::ServerContext *context, const ExportMetricsServiceRequest *request,
                      ExportMetricsServiceResponse *response) override;

private:
  OtelSourceDriverCpp &driver;
};

#endif
