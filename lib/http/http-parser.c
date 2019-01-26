/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady
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

#include "http-parser.h"
#include "http-message-internal.h"
#include "http-parser/http_parser.h"

#define HPE_UPGRADE_NOT_SUPPORTED 501

struct _HTTPParser
{
  http_parser parser;
  http_parser_settings parser_settings;

  HTTPMessage *current_message;
  gboolean message_complete;

  GString *current_header_field_name;
  GString *current_header_field_value;
};

static gint _message_begin(http_parser *parser);
static gint _headers_complete(http_parser *parser);
static gint _message_complete(http_parser *parser);
static gint _url(http_parser *parser, const gchar *data, gsize length);
static gint _header_field(http_parser *parser, const gchar *data, gsize length);
static gint _header_value(http_parser *parser, const gchar *data, gsize length);
static gint _body(http_parser *parser, const gchar *data, gsize length);

GQuark
http_parser_error_quark(void)
{
  return g_quark_from_static_string("http-parser-error-quark");
}

static HTTPParser *
_http_parser_new(enum http_parser_type parser_type)
{
  HTTPParser *self = g_new0(HTTPParser, 1);
  http_parser_init(&self->parser, parser_type);
  self->parser.data = self;

  self->parser_settings = (http_parser_settings)
  {
    .on_message_begin = _message_begin,
     .on_url = _url,
      .on_status = NULL,
       .on_header_field = _header_field,
        .on_header_value = _header_value,
         .on_headers_complete = _headers_complete,
          .on_body = _body,
           .on_message_complete = _message_complete,
            .on_chunk_header = NULL,
             .on_chunk_complete = NULL,
  };

  self->current_header_field_name = g_string_sized_new(32);
  self->current_header_field_value = g_string_sized_new(32);

  return self;
}

HTTPParser *
http_request_parser_new(void)
{
  return _http_parser_new(HTTP_REQUEST);
}

HTTPParser *
http_response_parser_new(void)
{
  return _http_parser_new(HTTP_RESPONSE);
}

void
http_parser_free(HTTPParser *self)
{
  http_message_free(self->current_message);
  g_string_free(self->current_header_field_name, TRUE);
  g_string_free(self->current_header_field_value, TRUE);
  g_free(self);
}

static HTTPMessage *
_create_message(enum http_parser_type type)
{
  switch (type)
    {
    case HTTP_REQUEST:
      return &http_request_new_empty()->super;
    case HTTP_RESPONSE:
      return &http_response_new_empty()->super;
    default:
      g_assert_not_reached();
    }
}

gboolean
http_parser_feed(HTTPParser *self, const gchar *data, gsize length, gsize *consumed_bytes)
{
  *consumed_bytes = http_parser_execute(&self->parser, &self->parser_settings, data, length);

  /* upgrade is not supported */
  if (self->parser.upgrade)
    return FALSE;

  /* pause is not an error */
  if (HTTP_PARSER_ERRNO(&self->parser) != HPE_PAUSED && *consumed_bytes != length)
    return FALSE;

  return TRUE;
}

gboolean
http_parser_signal_end_of_stream(HTTPParser *self)
{
  return http_parser_execute(&self->parser, &self->parser_settings, NULL, 0) == 0;
}

void
http_parser_skip_message(HTTPParser *self)
{
  http_message_free(self->current_message);
  self->current_message = NULL;
  self->message_complete = FALSE;
  http_parser_pause(&self->parser, 0);
}

gboolean
http_parser_is_message_complete(const HTTPParser *self)
{
  return self->message_complete;
}

HTTPMessage *
http_parser_steal_message(HTTPParser *self)
{
  if (!self->message_complete)
    return NULL;

  HTTPMessage *msg = self->current_message;
  self->current_message = NULL;
  self->message_complete = FALSE;
  http_parser_pause(&self->parser, 0);

  return msg;
}

GError *
http_parser_get_last_error(const HTTPParser *self)
{
  enum http_errno http_errno = HTTP_PARSER_ERRNO(&self->parser);
  if (http_errno != HPE_OK)
    return g_error_new(HTTP_PARSER_ERROR, http_errno, "%s", http_errno_description(http_errno));

  if (self->parser.upgrade)
    return g_error_new(HTTP_PARSER_ERROR, HPE_UPGRADE_NOT_SUPPORTED, "HTTP upgrade is not supported");

  return NULL;
}

static gboolean
http_parser_previous_header_exists(HTTPParser *self)
{
  return self->current_header_field_value->len != 0 && self->current_header_field_name->len != 0;
}

static void
http_parser_reset_header(HTTPParser *self)
{
  g_string_truncate(self->current_header_field_name, 0);
  g_string_truncate(self->current_header_field_value, 0);
}

static void
http_parser_finalize_previous_header(HTTPParser *self)
{
  if (!http_parser_previous_header_exists(self))
    return;

  http_message_add_header_normalized_in_place(self->current_message,
                                              self->current_header_field_name, self->current_header_field_value);

  http_parser_reset_header(self);
}

static gint
_message_begin(http_parser *parser)
{
  HTTPParser *self = parser->data;

  if (!self->current_message)
    self->current_message = _create_message(self->parser.type);

  return 0;
}

static gint
_headers_complete(http_parser *parser)
{
  HTTPParser *self = parser->data;

  http_parser_finalize_previous_header(self);

  if (parser->type == HTTP_RESPONSE)
    {
      HTTPResponse *response = (HTTPResponse *) self->current_message;
      http_response_set_status_code(response, parser->status_code);
    }

  if (parser->type == HTTP_REQUEST)
    {
      HTTPRequest *request = (HTTPRequest *) self->current_message;
      http_request_set_method(request, http_method_str(parser->method));
    }

  http_message_set_http_version(self->current_message, parser->http_major, parser->http_minor);
  return 0;
}

static gint
_message_complete(http_parser *parser)
{
  HTTPParser *self = parser->data;

  self->message_complete = TRUE;
  http_parser_pause(&self->parser, 1);
  return 0;
}

static gint
_url(http_parser *parser, const gchar *data, gsize length)
{
  HTTPParser *self = parser->data;
  HTTPRequest *request = (HTTPRequest *) self->current_message;

  http_request_append_url(request, data, length);
  return 0;
}

static gint
_header_field(http_parser *parser, const gchar *data, gsize length)
{
  HTTPParser *self = parser->data;

  http_parser_finalize_previous_header(self);

  g_string_append_len(self->current_header_field_name, data, length);
  return 0;
}

static gint
_header_value(http_parser *parser, const gchar *data, gsize length)
{
  HTTPParser *self = parser->data;

  g_string_append_len(self->current_header_field_value, data, length);
  return 0;
}

static gint
_body(http_parser *parser, const gchar *data, gsize length)
{
  HTTPParser *self = parser->data;

  http_message_append_body(self->current_message, (guint8 *) data, length);
  return 0;
}
