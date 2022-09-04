/*
 * Copyright (c) 2022 László Várady
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */
#include "logproto-binary-test-server.h"
#include "messages.h"
#include "logmsg/logmsg.h"

#include <errno.h>
#include <ctype.h>
#include <string.h>

typedef enum
{
  LPBTS_FETCH,
  LPBTS_EXTRACT,
} LogProtoBinaryTestServerState;

typedef enum
{
  LPBTSSC_NEXT,
  LPBTSSC_STOP,
} LogProtoBinaryTestServerStateControl;

typedef struct _LogProtoBinaryTestServer
{
  LogProtoServer super;

  guchar *buffer;
  guint32 buffer_size, buffer_pos, buffer_end;

  LogProtoBinaryTestServerState state;
} LogProtoBinaryTestServer;

static LogProtoPrepareAction
log_proto_binary_test_server_prepare(LogProtoServer *s, GIOCondition *cond, gint *timeout G_GNUC_UNUSED)
{
  LogProtoBinaryTestServer *self = (LogProtoBinaryTestServer *) s;

  *cond = self->super.transport->cond;

  if (self->buffer_pos != self->buffer_end)
    return LPPA_FORCE_SCHEDULE_FETCH;

  /* if there's no pending I/O in the transport layer, then we want to do a read */
  if (*cond == 0)
    *cond = G_IO_IN;

  return LPPA_POLL_IO;
}

static gboolean
log_proto_binary_test_server_fetch_data(LogProtoBinaryTestServer *self,
                                        LogTransportAuxData *aux, LogProtoStatus *status)
{
  *status = LPS_SUCCESS;

  if (self->buffer_pos == self->buffer_end)
    self->buffer_pos = self->buffer_end = 0;

  gint rc = log_transport_read(self->super.transport, &self->buffer[self->buffer_end],
                              self->buffer_size - self->buffer_end, aux);

  if (rc > 0)
    {
      self->buffer_end += rc;
      return TRUE;
    }

  if (rc == 0)
    {
      msg_trace("EOF occurred while reading",
                evt_tag_int(EVT_TAG_FD, self->super.transport->fd));
      log_transport_aux_data_reinit(aux);
      *status = LPS_EOF;
      return FALSE;
    }

  if (errno == EAGAIN)
    {
      return FALSE;
    }

  msg_error("Error reading data",
            evt_tag_int("fd", self->super.transport->fd),
            evt_tag_error("error"));
  log_transport_aux_data_reinit(aux);
  *status = LPS_ERROR;
  return FALSE;
}

static void
_adjust_buffer(LogProtoBinaryTestServer *self)
{
  memmove(self->buffer, &self->buffer[self->buffer_pos], self->buffer_end - self->buffer_pos);
  self->buffer_end = self->buffer_end - self->buffer_pos;
  self->buffer_pos = 0;
}

static void
_ensure_buffer(LogProtoBinaryTestServer *self)
{
  if (G_LIKELY(self->buffer))
    return;

  self->buffer_size = self->super.options->init_buffer_size;
  self->buffer = g_malloc(self->buffer_size);
}

static gboolean
log_proto_binary_test_server_extract_data(LogProtoBinaryTestServer *self , LogProtoStatus *status, LogMessage **msg)
{
  /* reads binary garbage and creates messages out of it every 5 bytes */
  const guint garbage_size = 5;

  guchar *data = self->buffer + self->buffer_pos;
  guint32 data_len = self->buffer_end - self->buffer_pos;

  if (data_len < garbage_size)
    return FALSE;

  LogMessage *m = log_msg_new_empty();
  log_msg_set_value_by_name(m, "binary", (const gchar *) data, garbage_size);
  log_msg_set_value(m, LM_V_MESSAGE, "this is a test message", -1);

  self->buffer_pos += garbage_size;

  *msg = m;
  *status = LPS_SUCCESS;
  return TRUE;
}

static LogProtoBinaryTestServerStateControl
_on_fetch(LogProtoBinaryTestServer *self, LogMessage **msg, LogTransportAuxData *aux, Bookmark *bookmark,
          LogProtoStatus *status)
{
  if (!log_proto_binary_test_server_fetch_data(self, aux, status))
    return LPBTSSC_STOP;

  self->state = LPBTS_EXTRACT;
  return LPBTSSC_NEXT;
}

static LogProtoBinaryTestServerStateControl
_on_extract(LogProtoBinaryTestServer *self, LogMessage **msg, LogTransportAuxData *aux, Bookmark *bookmark,
            LogProtoStatus *status)
{
  if (log_proto_binary_test_server_extract_data(self, status, msg))
    return LPBTSSC_STOP;

  self->state = LPBTS_FETCH;
  return LPBTSSC_NEXT;
}

static LogProtoStatus
log_proto_binary_test_server_fetch(LogProtoServer *s, LogMessage **msg, LogTransportAuxData *aux, Bookmark *bookmark)
{
  LogProtoBinaryTestServer *self = (LogProtoBinaryTestServer *) s;
  LogProtoStatus status;

  _ensure_buffer(self);

  while (TRUE)
    {
      _adjust_buffer(self);

      LogProtoBinaryTestServerStateControl ctrl;
      switch (self->state)
        {
        case LPBTS_FETCH:
          ctrl = _on_fetch(self, msg, aux, bookmark, &status);
          break;
        case LPBTS_EXTRACT:
          ctrl = _on_extract(self, msg, aux, bookmark, &status);
          break;
        default:
          ctrl = LPBTSSC_NEXT;
          break;
        }

      if (ctrl == LPBTSSC_STOP)
        return status;
    }
}

static void
log_proto_binary_test_server_free(LogProtoServer *s)
{
  LogProtoBinaryTestServer *self = (LogProtoBinaryTestServer *) s;
  g_free(self->buffer);

  log_proto_server_free_method(s);
}

LogProtoServer *
log_proto_binary_test_server_new(LogTransport *transport, const LogProtoServerOptions *options)
{
  LogProtoBinaryTestServer *self = g_new0(LogProtoBinaryTestServer, 1);

  log_proto_server_init(&self->super, transport, options);

  self->super.prepare = log_proto_binary_test_server_prepare;

  self->super.is_structured = TRUE;
  self->super.fetch_structured = log_proto_binary_test_server_fetch;

  self->super.free_fn = log_proto_binary_test_server_free;
  return &self->super;
}
