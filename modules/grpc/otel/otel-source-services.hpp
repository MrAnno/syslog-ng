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

#include "otel-servicecall.hpp"
#include "otel-source.hpp"
#include "otel-protobuf-parser.hpp"

#include <grpcpp/grpcpp.h>

namespace otel
{


class AsyncServiceCall
{
public:
  virtual void Proceed(bool ok) = 0;
};

template <class S, class Req, class Res>
class OtelAsyncServiceCall final : public AsyncServiceCall
{
public:
  void Proceed(bool ok) override;

public:
  OtelAsyncServiceCall(OtelSourceDriverCpp &driver_, S *service_, grpc::ServerCompletionQueue *cq_)
    : driver(driver_), service(service_), responder(&ctx), cq(cq_), status(PROCESS)
  {
    service->RequestExport(&ctx, &request, &responder, cq, cq, this);
  }

private:
  OtelSourceDriverCpp &driver;
  S *service;
  grpc::ServerAsyncResponseWriter<Res> responder;
  Req request;
  Res response;

  grpc::ServerCompletionQueue *cq;
  grpc::ServerContext ctx;

  enum CallStatus { PROCESS, FINISH };
  CallStatus status;
};

}

template <> void
otel::OtelTraceServiceCall::Proceed(bool ok)
{
  if (status == FINISH || !ok)
    {
      delete this;
      return;
    }

  new OtelTraceServiceCall(driver, service, cq);

  for (const ResourceSpans &resource_spans : request.resource_spans())
    {
      const Resource &resource = resource_spans.resource();
      const std::string &resource_logs_schema_url = resource_spans.schema_url();

      for (const ScopeSpans &scope_spans : resource_spans.scope_spans())
        {
          const InstrumentationScope &scope = scope_spans.scope();
          const std::string &scope_logs_schema_url = scope_spans.schema_url();

          for (const Span &span : scope_spans.spans())
            {
              LogMessage *msg = create_log_msg_with_metadata(ctx.peer(), resource,
                                                             resource_logs_schema_url, scope, scope_logs_schema_url);
              parse_Span(msg, span);
              if (!driver.post(msg)) ; // TODO: respond with "try-again-later"
            }
        }
    }

  status = FINISH;
  responder.Finish(response, grpc::Status::OK, this);
}

template <> void
otel::OtelLogsServiceCall::Proceed(bool ok)
{
  if (status == FINISH || !ok)
    {
      delete this;
      return;
    }

  new OtelLogsServiceCall(driver, service, cq);

  for (const ResourceLogs &resource_logs : request.resource_logs())
    {
      const Resource &resource = resource_logs.resource();
      const std::string &resource_logs_schema_url = resource_logs.schema_url();

      for (const ScopeLogs &scope_logs : resource_logs.scope_logs())
        {
          const InstrumentationScope &scope = scope_logs.scope();
          const std::string &scope_logs_schema_url = scope_logs.schema_url();

          for (const LogRecord &log_record : scope_logs.log_records())
            {
              LogMessage *msg = create_log_msg_with_metadata(ctx.peer(), resource, resource_logs_schema_url,
                                                             scope, scope_logs_schema_url);
              parse_LogRecord(msg, log_record);
              if (!driver.post(msg)) ; // TODO: respond with "try-again-later"
            }
        }
    }

  status = FINISH;
  responder.Finish(response, grpc::Status::OK, this);
}

template <> void
otel::OtelMetricsServiceCall::Proceed(bool ok)
{
  if (status == FINISH || !ok)
    {
      delete this;
      return;
    }

  new OtelMetricsServiceCall(driver, service, cq);

  for (const ResourceMetrics &resource_metrics : request.resource_metrics())
    {
      const Resource &resource = resource_metrics.resource();
      const std::string &resource_logs_schema_url = resource_metrics.schema_url();

      for (const ScopeMetrics &scope_metrics : resource_metrics.scope_metrics())
        {
          const InstrumentationScope &scope = scope_metrics.scope();
          const std::string &scope_logs_schema_url = scope_metrics.schema_url();

          for (const Metric &metric : scope_metrics.metrics())
            {
              LogMessage *msg = create_log_msg_with_metadata(ctx.peer(), resource,
                                                             resource_logs_schema_url, scope, scope_logs_schema_url);
              parse_Metric(msg, metric);
              if (!driver.post(msg)) ; // TODO: respond with "try-again-later"
            }
        }
    }

  status = FINISH;
  responder.Finish(response, grpc::Status::OK, this);
}

#endif
