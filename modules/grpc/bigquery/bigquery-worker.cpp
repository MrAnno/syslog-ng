/*
 * Copyright (c) 2023 László Várady
 * Copyright (c) 2023 Attila Szakacs
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

#include "bigquery-worker.hpp"
#include "bigquery-dest.hpp"

#include "compat/cpp-start.h"
#include "logthrdest/logthrdestdrv.h"
#include "compat/cpp-end.h"

#include <sstream>
#include <chrono>

#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <google/protobuf/dynamic_message.h>

using syslog_ng::bigquery::DestinationWorker;
using syslog_ng::bigquery::DestinationDriver;

struct _BigQueryDestWorker
{
  LogThreadedDestWorker super;
  DestinationWorker *cpp;
};

DestinationWorker::DestinationWorker(BigQueryDestWorker *s) : super(s), msg_factory(&descriptor_pool)
{
  DestinationDriver *owner = this->get_owner();

  std::stringstream table_name;
  table_name << "projects/" << owner->get_project()
             << "/datasets/" << owner->get_dataset()
             << "/tables/" << owner->get_table();
  this->table = table_name.str();
}

DestinationWorker::~DestinationWorker()
{
}

bool
DestinationWorker::init()
{
  DestinationDriver *owner = this->get_owner();

  this->channel = grpc::CreateChannel(owner->get_url(), grpc::GoogleDefaultCredentials());
  if (!this->channel)
    {
      msg_error("Error creating BigQuery gRPC channel", log_pipe_location_tag((LogPipe *) this->super->super.owner));
      return false;
    }

  this->stub = google::cloud::bigquery::storage::v1::BigQueryWrite().NewStub(channel);

  this->construct_msg_prototype();

  return log_threaded_dest_worker_init_method(&this->super->super);
}

void
DestinationWorker::deinit()
{
  log_threaded_dest_worker_deinit_method(&this->super->super);
}

bool
DestinationWorker::connect()
{
  this->construct_write_stream();
  this->batch_writer_ctx = std::make_unique<grpc::ClientContext>();
  this->batch_writer = this->stub->AppendRows(this->batch_writer_ctx.get());

  this->prepare_batch();

  msg_debug("Connecting to BigQuery", log_pipe_location_tag((LogPipe *) this->super->super.owner));

  std::chrono::system_clock::time_point connect_timeout =
    std::chrono::system_clock::now() + std::chrono::seconds(10);

  if (!this->channel->WaitForConnected(connect_timeout))
    return false;

  this->connected = true;
  return true;
}

void
DestinationWorker::disconnect()
{
  if (!this->connected)
    return;

  if (!this->batch_writer->WritesDone())
    msg_warning("Error closing BigQuery write stream, writes may have been unsuccessful",
                log_pipe_location_tag((LogPipe *) this->super->super.owner));

  grpc::Status status = this->batch_writer->Finish();
  if (!status.ok() && status.error_code() != grpc::StatusCode::INVALID_ARGUMENT)
    {
      msg_warning("Error closing BigQuery stream", evt_tag_str("error", status.error_message().c_str()),
                  evt_tag_str("details", status.error_details().c_str()),
                  evt_tag_int("code", status.error_code()),
                  log_pipe_location_tag((LogPipe *) this->super->super.owner));
    }

  grpc::ClientContext ctx;
  google::cloud::bigquery::storage::v1::FinalizeWriteStreamRequest finalize_request;
  google::cloud::bigquery::storage::v1::FinalizeWriteStreamResponse finalize_response;
  finalize_request.set_name(write_stream.name());

  status = this->stub->FinalizeWriteStream(&ctx, finalize_request, &finalize_response);
  if (!status.ok())
    {
      msg_warning("Error finalizing BigQuery write stream", evt_tag_str("error", status.error_message().c_str()),
                  evt_tag_str("details", status.error_details().c_str()),
                  log_pipe_location_tag((LogPipe *) this->super->super.owner));
    }

  this->connected = false;
}

void
DestinationWorker::prepare_batch()
{
  this->batch_size = 0;
  this->current_batch = google::cloud::bigquery::storage::v1::AppendRowsRequest{};

  this->current_batch.set_write_stream(write_stream.name());
  this->current_batch.set_trace_id("syslog-ng-bigquery");
  google::cloud::bigquery::storage::v1::AppendRowsRequest_ProtoData *proto_rows =
    this->current_batch.mutable_proto_rows();
  google::cloud::bigquery::storage::v1::ProtoSchema *schema = proto_rows->mutable_writer_schema();
  this->descriptor->CopyTo(schema->mutable_proto_descriptor());
}

LogThreadedResult
DestinationWorker::insert(LogMessage *msg)
{
  google::cloud::bigquery::storage::v1::ProtoRows *rows = this->current_batch.mutable_proto_rows()->mutable_rows();

  google::protobuf::Message *message = this->msg_prototype->New();
  const google::protobuf::Reflection *reflection = message->GetReflection();

  reflection->SetString(message, this->message_field_descriptor, log_msg_get_value(msg, LM_V_MESSAGE, NULL));
  reflection->SetString(message, this->event_source_field_descriptor, log_msg_get_value(msg, LM_V_PROGRAM, NULL));
  reflection->SetString(message, this->time_field_descriptor, log_msg_get_value_by_name(msg, "ISODATE", NULL));
  reflection->SetString(message, this->host_field_descriptor, log_msg_get_value(msg, LM_V_HOST, NULL));
  reflection->SetString(message, this->extradata_field_descriptor, log_msg_get_value_by_name(msg, "EXTRADATA", NULL));
  reflection->SetString(message, this->ident_field_descriptor, log_msg_get_value_by_name(msg, "IDENT", NULL));
  reflection->SetString(message, this->msgid_field_descriptor, log_msg_get_value_by_name(msg, "MSGID", NULL));
  reflection->SetString(message, this->pid_field_descriptor, log_msg_get_value(msg, LM_V_PID, NULL));

  this->batch_size++;
  rows->add_serialized_rows(message->SerializeAsString());

  delete message;

  return LTR_QUEUED;
}

LogThreadedResult
DestinationWorker::flush(LogThreadedFlushMode mode)
{
  if (this->batch_size == 0)
    return LTR_SUCCESS;

  if (!this->batch_writer->Write(current_batch))
    {
      msg_error("Error writing BigQuery batch", log_pipe_location_tag((LogPipe *) this->super->super.owner));
      return LTR_ERROR;
    }

  google::cloud::bigquery::storage::v1::AppendRowsResponse append_rows_response;
  if (!this->batch_writer->Read(&append_rows_response))
    {
      msg_error("Error reading BigQuery batch response", log_pipe_location_tag((LogPipe *) this->super->super.owner));
      return LTR_ERROR;
    }

  this->prepare_batch();
  return LTR_SUCCESS;
}

void
DestinationWorker::construct_write_stream()
{
  grpc::ClientContext ctx;
  google::cloud::bigquery::storage::v1::CreateWriteStreamRequest create_write_stream_request;
  google::cloud::bigquery::storage::v1::WriteStream wstream;

  create_write_stream_request.set_parent(this->table);
  create_write_stream_request.mutable_write_stream()->set_type(
    google::cloud::bigquery::storage::v1::WriteStream_Type_COMMITTED);

  stub->CreateWriteStream(&ctx, create_write_stream_request, &wstream);

  this->write_stream = wstream;
}

void
DestinationWorker::construct_msg_prototype()
{
  google::protobuf::FileDescriptorProto file_descriptor_proto;
  file_descriptor_proto.set_name("bigquery_record.proto");
  file_descriptor_proto.set_syntax("proto2");
  google::protobuf::DescriptorProto *descriptor_proto = file_descriptor_proto.add_message_type();
  descriptor_proto->set_name("BigQueryRecord");

  google::protobuf::FieldDescriptorProto *message_field_descriptor_proto = descriptor_proto->add_field();
  message_field_descriptor_proto->set_name("message");
  message_field_descriptor_proto->set_number(1);
  message_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *event_source_field_descriptor_proto = descriptor_proto->add_field();
  event_source_field_descriptor_proto->set_name("event_source");
  event_source_field_descriptor_proto->set_number(2);
  event_source_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *time_field_descriptor_proto = descriptor_proto->add_field();
  time_field_descriptor_proto->set_name("time");
  time_field_descriptor_proto->set_number(3);
  time_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *host_field_descriptor_proto = descriptor_proto->add_field();
  host_field_descriptor_proto->set_name("host");
  host_field_descriptor_proto->set_number(4);
  host_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *extradata_field_descriptor_proto = descriptor_proto->add_field();
  extradata_field_descriptor_proto->set_name("extradata");
  extradata_field_descriptor_proto->set_number(5);
  extradata_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *ident_field_descriptor_proto = descriptor_proto->add_field();
  ident_field_descriptor_proto->set_name("ident");
  ident_field_descriptor_proto->set_number(6);
  ident_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *msgid_field_descriptor_proto = descriptor_proto->add_field();
  msgid_field_descriptor_proto->set_name("msgid");
  msgid_field_descriptor_proto->set_number(7);
  msgid_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  google::protobuf::FieldDescriptorProto *pid_field_descriptor_proto = descriptor_proto->add_field();
  pid_field_descriptor_proto->set_name("pid");
  pid_field_descriptor_proto->set_number(8);
  pid_field_descriptor_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_STRING);

  const google::protobuf::FileDescriptor *file_descriptor = this->descriptor_pool.BuildFile(file_descriptor_proto);

  this->descriptor = file_descriptor->FindMessageTypeByName("BigQueryRecord");

  this->message_field_descriptor = this->descriptor->FindFieldByName("message");
  this->event_source_field_descriptor = this->descriptor->FindFieldByName("event_source");
  this->time_field_descriptor = this->descriptor->FindFieldByName("time");
  this->host_field_descriptor = this->descriptor->FindFieldByName("host");
  this->extradata_field_descriptor = this->descriptor->FindFieldByName("extradata");
  this->ident_field_descriptor = this->descriptor->FindFieldByName("ident");
  this->msgid_field_descriptor = this->descriptor->FindFieldByName("msgid");
  this->pid_field_descriptor = this->descriptor->FindFieldByName("pid");

  this->msg_prototype = msg_factory.GetPrototype(this->descriptor);
}

DestinationDriver *
DestinationWorker::get_owner()
{
  return bigquery_dd_get_cpp((BigQueryDestDriver *) this->super->super.owner);
}

/* C Wrappers */

static LogThreadedResult
_insert(LogThreadedDestWorker *s, LogMessage *msg)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  return self->cpp->insert(msg);
}

static LogThreadedResult
_flush(LogThreadedDestWorker *s, LogThreadedFlushMode mode)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  return self->cpp->flush(mode);
}

static gboolean
_connect(LogThreadedDestWorker *s)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  return self->cpp->connect();
}

static void
_disconnect(LogThreadedDestWorker *s)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  self->cpp->disconnect();
}

static gboolean
_init(LogThreadedDestWorker *s)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  return self->cpp->init();
}

static void
_deinit(LogThreadedDestWorker *s)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  self->cpp->deinit();
}

static void
_free(LogThreadedDestWorker *s)
{
  BigQueryDestWorker *self = (BigQueryDestWorker *) s;
  delete self->cpp;

  log_threaded_dest_worker_free_method(s);
}

LogThreadedDestWorker *
bigquery_dw_new(LogThreadedDestDriver *o, gint worker_index)
{
  BigQueryDestWorker *self = g_new0(BigQueryDestWorker, 1);

  log_threaded_dest_worker_init_instance(&self->super, o, worker_index);

  self->cpp = new DestinationWorker(self);

  self->super.init = _init;
  self->super.deinit = _deinit;
  self->super.connect = _connect;
  self->super.disconnect = _disconnect;
  self->super.insert = _insert;
  self->super.flush = _flush;
  self->super.free_fn = _free;

  return &self->super;
}
