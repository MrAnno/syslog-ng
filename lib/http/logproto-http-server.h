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

#ifndef LOGPROTO_HTTP_SERVER_H_INCLUDED
#define LOGPROTO_HTTP_SERVER_H_INCLUDED

#include "logproto/logproto-server.h"
#include "http/http-message.h"

#include <glib.h>

//TODO: LogProtoServer + Logreader: structured fetch interface

typedef struct _LogProtoHTTPServer LogProtoHTTPServer;

//TODO: runs in the reader thread, mention it on the other side
typedef GQueue *(*LPHTTPExtractMessagesFunc)(const HTTPRequest *http_request, gpointer user_data);
typedef HTTPResponse *(*LPHTTPCreateResponse)(const HTTPRequest *http_request, gpointer user_data);

LogProtoServer *log_proto_http_server_new(LogTransport *transport, const LogProtoServerOptions *options);
void log_proto_http_server_set_extract_messages(LogProtoHTTPServer *self, LPHTTPExtractMessagesFunc extract_messages,
                                                gpointer user_data);
void log_proto_http_server_set_create_response(LogProtoHTTPServer *self, LPHTTPCreateResponse create_response,
                                               gpointer user_data);

#endif