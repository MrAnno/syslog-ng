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

GQueue *
_extract_messages(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  EHTTPSourceDriver *self = (EHTTPSourceDriver *) connection->owner;
  MsgFormatOptions *parse_option = &self->super.reader_options.parse_options;

  http_request_null_terminate_body(http_request);
  const GByteArray *body = http_request_get_body(http_request);
  if (!body)
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

HTTPResponse *
_create_response(HTTPRequest *http_request, HTTPSourceConnection *connection)
{
  HTTPResponse *http_response = http_response_new_empty();
  http_response_set_http_version(http_response, 1, 1);
  http_response_set_status_code(http_response, HTTP_OK);
  http_response_add_header(http_response, "Content-Type", "text/html");
  return http_response;
}

EHTTPSourceDriver *
ehttp_sd_new(GlobalConfig *cfg)
{
  EHTTPSourceDriver *self = g_new0(EHTTPSourceDriver, 1);
  http_sd_init_instance(&self->super, _socket_options_inet_new(), _transport_mapper_network_new(), cfg);

  self->super.extract_log_messages = _extract_messages;
  self->super.create_response = _create_response;

  return self;
}
