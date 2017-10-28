/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 László Várady
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

#include "http-message.h"
#include "http-message-internal.h"

#include <string.h>

#define INITIAL_RAW_HEADERS_BUFFER_SIZE 64
#define INITIAL_URL_BUFFER_SIZE 64
#define HTTP_HEADER_KEY_VALUE_SEPARATOR ": "
#define HTTP_HEADER_SEPARATOR "\r\n"

struct _HTTPMessage
{
  gushort http_major;
  gushort http_minor;

  GString *raw_headers;
  GHashTable *header_positions;

  GByteArray *body;

  void (*free)(HTTPMessage *self);
};

typedef struct _HTTPHeaderKey
{
  const gchar **buffer;
  gsize offset;
  gsize length;
} HTTPHeaderKey;

typedef struct _HTTPHeaderValuePosition
{
  gsize offset;
  gsize length;
} HTTPHeaderValuePosition;

static const gchar *
http_header_get_key_start(const HTTPHeaderKey *key)
{
  return (*key->buffer) + key->offset;
}

static gsize
http_header_get_key_length(const HTTPHeaderKey *key)
{
  return key->length;
}

static guint
_http_header_key_hash(gconstpointer k)
{
  const HTTPHeaderKey *key = k;
  guint32 hash = 5381;

  const signed char *skey = (const signed char *) http_header_get_key_start(key);
  for (gsize i = 0; i < key->length; ++i)
    hash = (hash << 5) + hash + skey[i];

  return hash;
}

static gboolean
_http_header_key_equal(gconstpointer k1, gconstpointer k2)
{
  const HTTPHeaderKey *key1 = k1;
  const HTTPHeaderKey *key2 = k2;

  if (http_header_get_key_length(key1) != http_header_get_key_length(key2))
    return FALSE;

  return strncmp(http_header_get_key_start(key1), http_header_get_key_start(key2),
                 http_header_get_key_length(key1)) == 0;
}


static void
http_message_new_method(HTTPMessage *self)
{
  self->raw_headers = g_string_sized_new(INITIAL_RAW_HEADERS_BUFFER_SIZE);
  self->header_positions = g_hash_table_new_full(_http_header_key_hash, _http_header_key_equal, g_free, g_free);
  self->body = g_byte_array_new();
}

static void
http_message_free_method(HTTPMessage *self)
{
  g_string_free(self->raw_headers, TRUE);
  g_hash_table_destroy(self->header_positions);
  g_byte_array_free(self->body, TRUE);
  g_free(self);
}

void
http_message_set_http_version(HTTPMessage *self, gushort major, gushort minor)
{
  self->http_major = major;
  self->http_minor = minor;
}

void
http_message_get_http_version(HTTPMessage *self, gushort *major, gushort *minor)
{
  *major = self->http_major;
  *minor = self->http_minor;
}

GString *
http_message_get_header_using_normalized_key(HTTPMessage *self, const gchar *normalized_key)
{
  HTTPHeaderKey key_wrapper =
  {
    .buffer = &normalized_key,
    .offset = 0,
    .length = strlen(normalized_key)
  };

  const HTTPHeaderValuePosition *value_position = g_hash_table_lookup(self->header_positions, &key_wrapper);
  if (!value_position)
    return NULL;

  return g_string_new_len(self->raw_headers->str + value_position->offset, value_position->length);
}

GString *
http_message_get_header(HTTPMessage *self, const gchar *key)
{
  gchar *normalized_key = g_ascii_strdown(key, strlen(key));
  GString *value = http_message_get_header_using_normalized_key(self, normalized_key);
  g_free(normalized_key);

  return value;
}

static void
http_message_add_normalized_header(HTTPMessage *self, const gchar *key, const gchar *value)
{
  HTTPHeaderKey *header_key = g_new(HTTPHeaderKey, 1);
  header_key->buffer = (const gchar **) &self->raw_headers->str;
  header_key->offset = self->raw_headers->len;

  g_string_append(self->raw_headers, key);

  header_key->length = self->raw_headers->len - header_key->offset;

  g_string_append(self->raw_headers, HTTP_HEADER_KEY_VALUE_SEPARATOR);

  HTTPHeaderValuePosition *value_position = g_new(HTTPHeaderValuePosition, 1);
  value_position->offset = self->raw_headers->len;

  g_string_append(self->raw_headers, value);

  value_position->length = self->raw_headers->len - value_position->offset;

  g_string_append(self->raw_headers, HTTP_HEADER_SEPARATOR);

  g_hash_table_insert(self->header_positions, header_key, value_position);
}

void
http_message_add_header(HTTPMessage *self, const gchar *key, const gchar *value)
{
  gchar *normalized_key = g_ascii_strdown(key, strlen(key));
  http_message_add_normalized_header(self, normalized_key, value);
  g_free(normalized_key);
}

void
http_message_take_body(HTTPMessage *self, GByteArray *body)
{
  g_byte_array_free(self->body, TRUE);
  self->body = body;
}

const GByteArray *
http_message_get_body(HTTPMessage *self)
{
  return self->body;
}

void
http_message_free(HTTPMessage *self)
{
  if (self)
    self->free(self);
}

void
http_message_append_body(HTTPMessage *self, const guint8 *data, gsize length)
{
  g_byte_array_append(self->body, data, length);
}

void
http_message_add_header_normalized_in_place(HTTPMessage *self, GString *key, GString *value)
{
  g_string_ascii_down(key);
  http_message_add_normalized_header(self, key->str, value->str);
}


/* HTTPRequest */

struct _HTTPRequest
{
  HTTPMessage super;
  GString *url;
  gchar *method;
};

static void
http_request_free_method(HTTPMessage *s)
{
  HTTPRequest *self = (HTTPRequest *) s;
  g_string_free(self->url, TRUE);
  g_free(self->method);
  http_message_free_method(s);
}

HTTPRequest *
http_request_new_empty(void)
{
  HTTPRequest *self = g_new0(HTTPRequest, 1);

  http_message_new_method(&self->super);
  self->super.free = http_request_free_method;

  self->url = g_string_sized_new(INITIAL_URL_BUFFER_SIZE);

  return self;
}

void
http_request_set_http_version(HTTPRequest *self, gushort major, gushort minor)
{
  http_message_set_http_version(&self->super, major, minor);
}

void
http_request_get_http_version(HTTPRequest *self, gushort *major, gushort *minor)
{
  http_message_get_http_version(&self->super, major, minor);
}

GString *
http_request_get_header(HTTPRequest *self, const gchar *key)
{
  return http_message_get_header(&self->super, key);
}

GString *
http_request_get_header_using_normalized_key(HTTPRequest *self, const gchar *normalized_key)
{
  return http_message_get_header_using_normalized_key(&self->super, normalized_key);
}

void
http_request_set_url(HTTPRequest *self, const gchar *url)
{
  g_string_assign(self->url, url);
}

const gchar *
http_request_get_url(HTTPRequest *self)
{
  return self->url->str;
}

void
http_request_take_body(HTTPRequest *self, GByteArray *body)
{
  http_message_take_body(&self->super, body);
}

const GByteArray *
http_request_get_body(HTTPRequest *self)
{
  return http_message_get_body(&self->super);
}

void
http_request_add_header(HTTPRequest *self, const gchar *key, const gchar *value)
{
  http_message_add_header(&self->super, key, value);
}

void
http_request_set_method(HTTPRequest *self, const gchar *method)
{
  g_free(self->method);
  self->method = g_strdup(method);
}

const gchar *
http_request_get_method(HTTPRequest *self)
{
  return self->method;
}

void
http_request_free(HTTPRequest *self)
{
  if (self)
    http_message_free(&self->super);
}

HTTPMessage *
http_request_upcast(HTTPRequest *self)
{
  return &self->super;
}

void
http_request_append_url(HTTPRequest *self, const gchar *data, gsize length)
{
  g_string_append_len(self->url, data, length);
}


/* HTTPResponse */

struct _HTTPResponse
{
  HTTPMessage super;
  HTTPStatusCode status_code;
};

static void
http_response_free_method(HTTPMessage *s)
{
  http_message_free_method(s);
}

HTTPResponse *
http_response_new_empty(void)
{
  HTTPResponse *self = g_new0(HTTPResponse, 1);

  http_message_new_method(&self->super);
  self->super.free = http_response_free_method;

  return self;
}

void
http_response_set_http_version(HTTPResponse *self, gushort major, gushort minor)
{
  http_message_set_http_version(&self->super, major, minor);
}

void
http_response_get_http_version(HTTPResponse *self, gushort *major, gushort *minor)
{
  http_message_get_http_version(&self->super, major, minor);
}

GString *
http_response_get_header(HTTPResponse *self, const gchar *key)
{
  return http_message_get_header(&self->super, key);
}

GString *
http_response_get_header_using_normalized_key(HTTPResponse *self, const gchar *normalized_key)
{
  return http_message_get_header_using_normalized_key(&self->super, normalized_key);
}

void
http_response_take_body(HTTPResponse *self, GByteArray *body)
{
  http_message_take_body(&self->super, body);
}

const GByteArray *
http_response_get_body(HTTPResponse *self)
{
  return http_message_get_body(&self->super);
}

void
http_response_add_header(HTTPResponse *self, const gchar *key, const gchar *value)
{
  http_message_add_header(&self->super, key, value);
}

void
http_response_set_status_code(HTTPResponse *self, HTTPStatusCode status_code)
{
  self->status_code = status_code;
}

HTTPStatusCode
http_response_get_status_code(HTTPResponse *self)
{
  return self->status_code;
}

void
http_response_free(HTTPResponse *self)
{
  if (self)
    http_message_free(&self->super);
}

HTTPMessage *
http_response_upcast(HTTPResponse *self)
{
  return &self->super;
}
