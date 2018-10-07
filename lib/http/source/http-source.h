/*
 * Copyright (c) 2018 Balabit
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

#ifndef HTTP_SOURCE_H_INCLUDED
#define HTTP_SOURCE_H_INCLUDED

#include "syslog-ng.h"
#include "http/source/socket-options.h"
#include "http/source/transport-mapper.h"
#include "http/http-message.h"
#include "driver.h"
#include "logreader.h"
#include "tlscontext.h"
#include "gsocket.h"

#include <iv.h>

typedef struct _HTTPSourceDriver HTTPSourceDriver;

typedef struct _HTTPSourceConnection
{
  LogPipe super;
  HTTPSourceDriver *owner;
  LogReader *reader;
  int sock;
  GSockAddr *peer_addr;
} HTTPSourceConnection;

struct _HTTPSourceDriver
{
  LogSrcDriver super;
  guint32 recvd_messages_are_local:1, connections_kept_alive_across_reloads:1, window_size_initialized:1;
  struct iv_fd listen_fd;
  gint fd;
  LogReaderOptions reader_options;
  GSockAddr *bind_addr;
  gint max_connections;
  gint num_connections;
  gint listen_backlog;
  GList *connections;
  _SocketOptions *socket_options;
  _TransportMapper *transport_mapper;

  gchar *bind_port;
  gchar *bind_ip;

  GQueue *(*extract_log_messages)(HTTPRequest *http_request, HTTPSourceConnection *connection);
  HTTPResponse *(*create_response)(HTTPRequest *http_request, HTTPSourceConnection *connection);
};

void http_sd_set_keep_alive(LogDriver *self, gint enable);
void http_sd_set_max_connections(LogDriver *self, gint max_connections);
void http_sd_set_listen_backlog(LogDriver *self, gint listen_backlog);
void http_sd_set_tls_context(LogDriver *s, TLSContext *tls_context);
void http_sd_set_localport(LogDriver *self, gchar *service);
void http_sd_set_localip(LogDriver *self, gchar *ip);

void http_sd_init_instance(HTTPSourceDriver *self, _SocketOptions *socket_options,
                           _TransportMapper *transport_mapper, GlobalConfig *cfg);
gboolean http_sd_init_method(LogPipe *s);
gboolean http_sd_deinit_method(LogPipe *s);
void http_sd_free_method(LogPipe *self);

#endif
