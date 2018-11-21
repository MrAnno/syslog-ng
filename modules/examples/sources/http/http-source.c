/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady <laszlo.varady@balabit.com>
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

#include "http-source.h"
#include "http/http-message.h"
#include "http/http-message-internal.h"
#include "http/source/transport-mapper-inet.h"
#include "http/source/socket-options-inet.h"
#include "logmsg/logmsg.h"

#include <string.h>
#include <strings.h>

#include <json.h>

typedef GQueue *(*EHTTPExtractMessageFunc)(HTTPRequest *http_request, HTTPSourceConnection *connection);

GQueue *
_extract_single_message(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) connection->owner;
  MsgFormatOptions *parse_option = &self->super.reader_options.parse_options;

  const GByteArray *body = http_message_get_body(&http_request->super);
  if (!body || !body->data)
    return NULL;

  GQueue *messages = g_queue_new();

  const gchar *message = (const gchar *) body->data;
  LogMessage *msg = log_msg_new(message, body->len, connection->peer_addr, parse_option);
  g_queue_push_tail(messages, msg);

  return messages;
}

GQueue *
_extract_messages_text(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) connection->owner;
  MsgFormatOptions *parse_option = &self->super.reader_options.parse_options;

  http_request_null_terminate_body(http_request);
  const GByteArray *body = http_message_get_body(&http_request->super);
  if (!body || !body->data)
    return NULL;

  GQueue *messages = g_queue_new();

  gchar *state;
  gchar *data = (gchar *) body->data;
  gchar *line = strtok_r(data, "\n", &state);

  while (line)
    {
      LogMessage *msg = log_msg_new(line, strlen(line), connection->peer_addr, parse_option);
      g_queue_push_tail(messages, msg);

      line = strtok_r(NULL, "\n", &state);
    }
  return messages;
}

static GQueue *
_extract_from_json(struct json_object *obj, GSockAddr *saddr, MsgFormatOptions *parse_options)
{
  struct json_object *messages_obj = obj;

  if (!json_object_is_type(obj, json_type_array))
    {
      struct json_object_iter itr;
      json_object_object_foreachC(obj, itr)
      {
        if (json_object_is_type(itr.val, json_type_array))
          {
            messages_obj = itr.val;
            break;
          }
      }

      if (messages_obj == obj)
        {
          msg_warning("Error extracting JSON messages, array object is not found");
          return NULL;
        }
    }

  GQueue *messages = g_queue_new();

  gsize messages_size = json_object_array_length(messages_obj);
  for (gsize i = 0; i < messages_size; ++i)
    {
      struct json_object *message_obj = json_object_array_get_idx(messages_obj, i);
      const gchar *message = json_object_get_string(message_obj);

      LogMessage *msg = log_msg_new(message, strlen(message), saddr, parse_options);
      g_queue_push_tail(messages, msg);
    }

  return messages;
}

GQueue *
_extract_messages_json(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) connection->owner;
  MsgFormatOptions *parse_option = &self->super.reader_options.parse_options;

  const GByteArray *body = http_message_get_body(&http_request->super);
  if (!body || !body->data)
    return NULL;

  struct json_tokener *tok = json_tokener_new();
  struct json_object *obj = json_tokener_parse_ex(tok, (const gchar *) body->data, body->len);
  if (tok->err != json_tokener_success || !obj)
    {
      msg_warning("Error parsing JSON messages");
      json_tokener_free(tok);
      return NULL;
    }
  json_tokener_free(tok);

  GQueue *messages = _extract_from_json(obj, connection->peer_addr, parse_option);
  json_object_put(obj);

  return messages;
}

static EHTTPExtractMessageFunc ehttp_extract_modes[] =
{
  _extract_single_message,
  _extract_messages_text,
  _extract_messages_json
};

HTTPResponse *
_create_response(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  HTTPResponse *http_response = http_response_new_empty();
  http_message_set_http_version(&http_response->super, 1, 1);
  http_response_set_status_code(http_response, HTTP_OK);

  GByteArray* body = g_byte_array_sized_new(32);
  const gchar *status_line = http_response_status_code_to_status_line(HTTP_OK);
  g_byte_array_append(body, (const guint8 *) status_line, strlen(status_line));

  http_message_take_body(&http_response->super, body);

  return http_response;
}

gboolean
ehttp_sd_set_mode(LogDriver *d, const gchar *mode)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) d;

  if (strcasecmp(mode, "single") == 0)
    self->mode = EHTTP_SINGLE;
  else if (strcasecmp(mode, "text") == 0)
    self->mode = EHTTP_TEXT;
  else if (strcasecmp(mode, "json") == 0)
    self->mode = EHTTP_JSON;
  else
    return FALSE;

  return TRUE;
}

gboolean
ehttp_sd_init(LogPipe *s)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) s;

  self->super.extract_log_messages = ehttp_extract_modes[self->mode];

  return http_sd_init_method(s);
}

EHTTPSourceDriver *
ehttp_sd_new(GlobalConfig *cfg)
{
  EHTTPSourceDriver *self = g_new0(EHTTPSourceDriver, 1);
  http_sd_init_instance(&self->super, _socket_options_inet_new(), _transport_mapper_network_new(), cfg);

  self->mode = EHTTP_SINGLE;

  self->super.create_response = _create_response;
  self->super.super.super.super.init = ehttp_sd_init;

  return self;
}
