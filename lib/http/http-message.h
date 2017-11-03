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

const gchar *http_response_status_code_to_status_line(HTTPStatusCode code);

/* gaps are not allowed */
#define HTTP_STATUS_MAP_200(T) \
  T(200, OK, OK) \
  T(201, CREATED, Created) \
  T(202, ACCEPTED, Accepted) \
  T(203, NON_AUTHORITATIVE_INFORMATION, Non-Authoritative Information) \
  T(204, NO_CONTENT, No Content) \
  T(205, RESET_CONTENT, Reset Content) \
  T(206, PARTIAL_CONTENT, Partial Content) \
  T(207, MULTI_STATUS, Multi-Status) \
  T(208, ALREADY_REPORTED, Already Reported)

#define HTTP_STATUS_MAP_300(T) \
  T(300, MULTIPLE_CHOICES, Multiple Choices) \
  T(301, MOVED_PERMANENTLY, Moved Permanently) \
  T(302, FOUND, Found) \
  T(303, SEE_OTHER, See Other) \
  T(304, NOT_MODIFIED, Not Modified) \
  T(305, USE_PROXY, Use Proxy) \
  T(306, SWITCH_PROXY, Switch Proxy) \
  T(307, TEMPORARY_REDIRECT, Temporary Redirect) \
  T(308, PERMANENT_REDIRECT, Permanent Redirect)

#define HTTP_STATUS_MAP_400(T) \
  T(400, BAD_REQUEST, Bad Request) \
  T(401, UNAUTHORIZED, Unauthorized) \
  T(402, PAYMENT_REQUIRED, Payment Required) \
  T(403, FORBIDDEN, Forbidden) \
  T(404, NOT_FOUND, Not Found) \
  T(405, METHOD_NOT_ALLOWED, Method Not Allowed) \
  T(406, NOT_ACCEPTABLE, Not Acceptable) \
  T(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required) \
  T(408, REQUEST_TIMEOUT, Request Timeout) \
  T(409, CONFLICT, Conflict) \
  T(410, GONE, Gone) \
  T(411, LENGTH_REQUIRED, Length Required) \
  T(412, PRECONDITION_FAILED, Precondition Failed) \
  T(413, PAYLOAD_TOO_LARGE, Payload Too Large) \
  T(414, URI_TOO_LONG, URI Too Long) \
  T(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type) \
  T(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable) \
  T(417, EXPECTATION_FAILED, Expectation Failed) \
  T(418, UNUSED_418, ) \
  T(419, UNUSED_419, ) \
  T(420, UNUSED_420, ) \
  T(421, MISDIRECTED_REQUEST, Misdirected Request) \
  T(422, UNPROCESSABLE_ENTITY, Unprocessable Entity) \
  T(423, LOCKED, Locked) \
  T(424, FAILED_DEPENDENCY, Failed Dependency) \
  T(425, UNUSED_425, ) \
  T(426, UPGRADE_REQUIRED, Upgrade Required) \
  T(427, UNUSED_427, ) \
  T(428, PRECONDITION_REQUIRED, Precondition Required) \
  T(429, TOO_MANY_REQUESTS, Too Many Requests) \
  T(430, UNUSED_430, ) \
  T(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)

#define HTTP_STATUS_MAP_500(T) \
  T(500, INTERNAL_SERVER_ERROR, Internal Server Error) \
  T(501, NOT_IMPLEMENTED, Not Implemented) \
  T(502, BAD_GATEWAY, Bad Gateway) \
  T(503, SERVICE_UNAVAILABLE, Service Unavailable) \
  T(504, GATEWAY_TIMEOUT, Gateway Timeout) \
  T(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported) \
  T(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates) \
  T(507, INSUFFICIENT_STORAGE, Insufficient Storage) \
  T(508, LOOP_DETECTED, Loop Detected) \
  T(509, UNUSED_509, ) \
  T(510, NOT_EXTENDED, Not Extended) \
  T(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

/*
 * HTTP_OK = 200,
 * HTTP_NOT_FOUND = 404,
 * ...
 */
enum _HTTPStatusCode
{
#define T(code, name, reason_phrase) HTTP_##name = code,
  HTTP_STATUS_MAP_200(T)
  HTTP_STATUS_MAP_300(T)
  HTTP_STATUS_MAP_400(T)
  HTTP_STATUS_MAP_500(T)
#undef T
};

#endif
