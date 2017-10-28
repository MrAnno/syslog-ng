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

#ifndef HTTP_MESSAGE_H_INCLUDED
#define HTTP_MESSAGE_H_INCLUDED

#include <glib.h>

typedef enum _HTTPStatusCode HTTPStatusCode;

typedef struct _HTTPMessage HTTPMessage;

void http_message_set_http_version(HTTPMessage *self, gushort major, gushort minor);
void http_message_get_http_version(HTTPMessage *self, gushort *major, gushort *minor);
void http_message_add_header(HTTPMessage *self, const gchar *key, const gchar *value);
GString *http_message_get_header(HTTPMessage *self, const gchar *key);
GString *http_message_get_header_using_normalized_key(HTTPMessage *self, const gchar *normalized_key);
void http_message_take_body(HTTPMessage *self, GByteArray *body);
const GByteArray *http_message_get_body(HTTPMessage *self);
void http_message_free(HTTPMessage *self);


typedef struct _HTTPRequest HTTPRequest;

HTTPRequest *http_request_new_empty(void);
HTTPMessage *http_request_upcast(HTTPRequest *self);
void http_request_set_http_version(HTTPRequest *self, gushort major, gushort minor);
void http_request_get_http_version(HTTPRequest *self, gushort *major, gushort *minor);
void http_request_add_header(HTTPRequest *self, const gchar *key, const gchar *value);
GString *http_request_get_header(HTTPRequest *self, const gchar *key);
GString *http_request_get_header_using_normalized_key(HTTPRequest *self, const gchar *normalized_key);
void http_request_take_body(HTTPRequest *self, GByteArray *body);
const GByteArray *http_request_get_body(HTTPRequest *self);
void http_request_set_url(HTTPRequest *self, const gchar *url);
const gchar *http_request_get_url(HTTPRequest *self);
void http_request_set_method(HTTPRequest *self, const gchar *method);
const gchar *http_request_get_method(HTTPRequest *self);
void http_request_free(HTTPRequest *self);

void http_request_null_terminate_body(HTTPRequest *self);


typedef struct _HTTPResponse HTTPResponse;

HTTPResponse *http_response_new_empty(void);
HTTPMessage *http_response_upcast(HTTPResponse *self);
void http_response_set_http_version(HTTPResponse *self, gushort major, gushort minor);
void http_response_get_http_version(HTTPResponse *self, gushort *major, gushort *minor);
void http_response_add_header(HTTPResponse *self, const gchar *key, const gchar *value);
GString *http_response_get_header(HTTPResponse *self, const gchar *key);
GString *http_response_get_header_using_normalized_key(HTTPResponse *self, const gchar *normalized_key);
void http_response_take_body(HTTPResponse *self, GByteArray *body);
const GByteArray *http_response_get_body(HTTPResponse *self);
void http_response_set_status_code(HTTPResponse *self, HTTPStatusCode status_code);
HTTPStatusCode http_response_get_status_code(HTTPResponse *self);
void http_response_free(HTTPResponse *self);


enum _HTTPStatusCode
{
  HTTP_CONTINUE = 100,
  HTTP_SWITCHING_PROTOCOLS = 101,
  HTTP_PROCESSING = 102,
  HTTP_OK = 200,
  HTTP_CREATED = 201,
  HTTP_ACCEPTED = 202,
  HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
  HTTP_NO_CONTENT = 204,
  HTTP_RESET_CONTENT = 205,
  HTTP_PARTIAL_CONTENT = 206,
  HTTP_MULTI_STATUS = 207,
  HTTP_ALREADY_REPORTED = 208,
  HTTP_IM_USED = 226,
  HTTP_MULTIPLE_CHOICES = 300,
  HTTP_MOVED_PERMANENTLY = 301,
  HTTP_FOUND = 302,
  HTTP_SEE_OTHER = 303,
  HTTP_NOT_MODIFIED = 304,
  HTTP_USE_PROXY = 305,
  HTTP_TEMPORARY_REDIRECT = 307,
  HTTP_PERMANENT_REDIRECT = 308,
  HTTP_BAD_REQUEST = 400,
  HTTP_UNAUTHORIZED = 401,
  HTTP_PAYMENT_REQUIRED = 402,
  HTTP_FORBIDDEN = 403,
  HTTP_NOT_FOUND = 404,
  HTTP_METHOD_NOT_ALLOWED = 405,
  HTTP_NOT_ACCEPTABLE = 406,
  HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
  HTTP_REQUEST_TIMEOUT = 408,
  HTTP_CONFLICT = 409,
  HTTP_GONE = 410,
  HTTP_LENGTH_REQUIRED = 411,
  HTTP_PRECONDITION_FAILED = 412,
  HTTP_PAYLOAD_TOO_LARGE = 413,
  HTTP_URI_TOO_LONG = 414,
  HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
  HTTP_RANGE_NOT_SATISFIABLE = 416,
  HTTP_EXPECTATION_FAILED = 417,
  HTTP_MISDIRECTED_REQUEST = 421,
  HTTP_UNPROCESSABLE_ENTITY = 422,
  HTTP_LOCKED = 423,
  HTTP_FAILED_DEPENDENCY = 424,
  HTTP_UPGRADE_REQUIRED = 426,
  HTTP_PRECONDITION_REQUIRED = 428,
  HTTP_TOO_MANY_REQUESTS = 429,
  HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
  HTTP_INTERNAL_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED = 501,
  HTTP_BAD_GATEWAY = 502,
  HTTP_SERVICE_UNAVAILABLE = 503,
  HTTP_GATEWAY_TIMEOUT = 504,
  HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
  HTTP_VARIANT_ALSO_NEGOTIATES = 506,
  HTTP_INSUFFICIENT_STORAGE = 507,
  HTTP_LOOP_DETECTED = 508,
  HTTP_NOT_EXTENDED = 510,
  HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511,
};

#endif
