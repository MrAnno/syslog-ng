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

#include "http-source.h"
#include "messages.h"
#include "fdhelpers.h"
#include "stats/stats-registry.h"
#include "mainloop.h"
#include "poll-fd-events.h"
#include "transport-mapper-inet.h"
#include "socket-options-inet.h"
#include "http/logproto-http-server.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCEPTS_AT_A_TIME 30

static gint
afinet_lookup_service_and_proto(const gchar *service, const gchar *proto)
{
  gchar *end;
  gint port;

  /* check if service is numeric */
  port = strtol(service, &end, 10);
  if ((*end != 0))
    {
      struct servent *se;

      /* service is not numeric, check if it's a service in /etc/services */
      se = getservbyname(service, proto);
      if (se)
        {
          port = ntohs(se->s_port);
        }
      else
        {
          msg_error("Error finding port number, falling back to default",
                    evt_tag_printf("service", "%s/%s", proto, service));
          return 0;
        }
    }
  return port;
}

static const gchar *
afinet_lookup_proto(gint protocol_number, gint sock_type)
{
  struct protoent *ipproto_ent = getprotobynumber(protocol_number);

  return ipproto_ent ? ipproto_ent->p_name
         : ((sock_type == SOCK_STREAM) ? "tcp" : "udp");
}

guint16
afinet_lookup_service(const _TransportMapper *transport_mapper, const gchar *service)
{
  const gchar *protocol_name = afinet_lookup_proto(transport_mapper->sock_proto, transport_mapper->sock_type);

  return afinet_lookup_service_and_proto(service, protocol_name);
}

gint
afinet_determine_port(const _TransportMapper *transport_mapper, const gchar *service_port)
{
  gint port = 0;

  if (!service_port)
    port = _transport_mapper_inet_get_server_port(transport_mapper);
  else
    port = afinet_lookup_service(transport_mapper, service_port);

  return port;
}

static void http_sd_close_connection(HTTPSourceDriver *self, HTTPSourceConnection *sc);

static gchar *
http_sc_stats_instance(HTTPSourceConnection *self)
{
  static gchar buf[256];
  gchar peer_addr[MAX_SOCKADDR_STRING];

  g_sockaddr_format(self->peer_addr, peer_addr, sizeof(peer_addr), GSA_ADDRESS_ONLY);
  g_snprintf(buf, sizeof(buf), "%s,%s", self->owner->transport_mapper->transport, peer_addr);
  return buf;
}

static LogTransport *
http_sc_construct_transport(HTTPSourceConnection *self, gint fd)
{
  return _transport_mapper_construct_log_transport(self->owner->transport_mapper, fd);
}

static gboolean
http_sc_init(LogPipe *s)
{
  HTTPSourceConnection *self = (HTTPSourceConnection *) s;
  LogTransport *transport;
  LogProtoServer *proto;

  if (!self->reader)
    {
      transport = http_sc_construct_transport(self, self->sock);
      if (!transport)
        return FALSE;

      proto = log_proto_http_server_new(transport, &self->owner->reader_options.proto_options.super);
      log_proto_http_server_set_extract_log_messages(proto,
                                                     (LPHTTPExtractLogMessagesFunc) self->owner->extract_log_messages,
                                                     self);
      log_proto_http_server_set_create_response(proto, (LPHTTPCreateResponseFunc) self->owner->create_response, self);

      if (!proto)
        {
          log_transport_free(transport);
          return FALSE;
        }

      self->reader = log_reader_new(s->cfg);
      log_reader_reopen(self->reader, proto, poll_fd_events_new(self->sock));
      log_reader_set_peer_addr(self->reader, self->peer_addr);
    }
  log_reader_set_options(self->reader, &self->super, &self->owner->reader_options, self->owner->super.super.id,
                         http_sc_stats_instance(self));

  log_pipe_append((LogPipe *) self->reader, s);
  if (log_pipe_init((LogPipe *) self->reader))
    {
      return TRUE;
    }
  else
    {
      log_pipe_unref((LogPipe *) self->reader);
      self->reader = NULL;
    }

  return FALSE;
}

static gboolean
http_sc_deinit(LogPipe *s)
{
  HTTPSourceConnection *self = (HTTPSourceConnection *) s;

  log_pipe_unref(&self->owner->super.super.super);
  self->owner = NULL;

  log_pipe_deinit((LogPipe *) self->reader);
  return TRUE;
}

static void
http_sc_notify(LogPipe *s, gint notify_code, gpointer user_data)
{
  HTTPSourceConnection *self = (HTTPSourceConnection *) s;

  switch (notify_code)
    {
    case NC_CLOSE:
    case NC_READ_ERROR:
    {
      if (self->owner->transport_mapper->sock_type == SOCK_STREAM)
        http_sd_close_connection(self->owner, self);
      break;
    }
    default:
      break;
    }
}

static void
http_sc_set_owner(HTTPSourceConnection *self, HTTPSourceDriver *owner)
{
  GlobalConfig *cfg = log_pipe_get_config(&owner->super.super.super);

  if (self->owner)
    log_pipe_unref(&self->owner->super.super.super);

  log_pipe_ref(&owner->super.super.super);
  self->owner = owner;
  self->super.expr_node = owner->super.super.super.expr_node;

  log_pipe_set_config(&self->super, cfg);
  if (self->reader)
    log_pipe_set_config((LogPipe *) self->reader, cfg);

  log_pipe_append(&self->super, &owner->super.super.super);
}

static void
http_sc_free(LogPipe *s)
{
  HTTPSourceConnection *self = (HTTPSourceConnection *) s;
  g_sockaddr_unref(self->peer_addr);
  log_pipe_free_method(s);
}

HTTPSourceConnection *
http_sc_new(GSockAddr *peer_addr, int fd, GlobalConfig *cfg)
{
  HTTPSourceConnection *self = g_new0(HTTPSourceConnection, 1);

  log_pipe_init_instance(&self->super, cfg);
  self->super.init = http_sc_init;
  self->super.deinit = http_sc_deinit;
  self->super.notify = http_sc_notify;
  self->super.free_fn = http_sc_free;
  self->peer_addr = g_sockaddr_ref(peer_addr);
  self->sock = fd;
  return self;
}

void
http_sd_add_connection(HTTPSourceDriver *self, HTTPSourceConnection *connection)
{
  self->connections = g_list_prepend(self->connections, connection);
}

static void
http_sd_kill_connection(HTTPSourceConnection *connection)
{
  log_pipe_deinit(&connection->super);

  log_pipe_unref((LogPipe *) connection->reader);
  connection->reader = NULL;

  log_pipe_unref(&connection->super);
}

static void
http_sd_kill_connection_list(GList *list)
{
  GList *l, *next;

  for (l = list; l; l = next)
    {
      HTTPSourceConnection *connection = (HTTPSourceConnection *) l->data;

      next = l->next;

      if (connection->owner)
        connection->owner->connections = g_list_remove(connection->owner->connections, connection);
      http_sd_kill_connection(connection);
    }
}

void
http_sd_set_keep_alive(LogDriver *s, gint enable)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  self->connections_kept_alive_across_reloads = enable;
}

void
http_sd_set_max_connections(LogDriver *s, gint max_connections)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  self->max_connections = max_connections;
}

void
http_sd_set_listen_backlog(LogDriver *s, gint listen_backlog)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  self->listen_backlog = listen_backlog;
}

static const gchar *
http_sd_format_name(const LogPipe *s)
{
  const HTTPSourceDriver *self = (const HTTPSourceDriver *)s;
  static gchar persist_name[1024];

  if (s->persist_name)
    {
      g_snprintf(persist_name, sizeof(persist_name), "http_sd.%s",
                 self->super.super.super.persist_name);
    }
  else
    {
      gchar buf[64];

      g_snprintf(persist_name, sizeof(persist_name), "http_sd.(%s,%s)",
                 (self->transport_mapper->sock_type == SOCK_STREAM) ? "stream" : "dgram",
                 g_sockaddr_format(self->bind_addr, buf, sizeof(buf), GSA_FULL));
    }

  return persist_name;
}

static const gchar *
http_sd_format_listener_name(const HTTPSourceDriver *self)
{
  static gchar persist_name[1024];

  g_snprintf(persist_name, sizeof(persist_name), "%s.listen_fd",
             http_sd_format_name((const LogPipe *)self));

  return persist_name;
}

static const gchar *
http_sd_format_connections_name(const HTTPSourceDriver *self)
{
  static gchar persist_name[1024];

  g_snprintf(persist_name, sizeof(persist_name), "%s.connections",
             http_sd_format_name((const LogPipe *)self));

  return persist_name;
}

static gboolean
http_sd_process_connection(HTTPSourceDriver *self, GSockAddr *client_addr, GSockAddr *local_addr, gint fd)
{
  gchar buf[MAX_SOCKADDR_STRING], buf2[MAX_SOCKADDR_STRING];

  if (self->num_connections >= self->max_connections)
    {
      msg_error("Number of allowed concurrent connections reached, rejecting connection",
                evt_tag_str("client", g_sockaddr_format(client_addr, buf, sizeof(buf), GSA_FULL)),
                evt_tag_str("local", g_sockaddr_format(local_addr, buf2, sizeof(buf2), GSA_FULL)),
                evt_tag_int("max", self->max_connections));
      return FALSE;
    }
  else
    {
      HTTPSourceConnection *conn;

      conn = http_sc_new(client_addr, fd, self->super.super.super.cfg);
      http_sc_set_owner(conn, self);
      if (log_pipe_init(&conn->super))
        {
          http_sd_add_connection(self, conn);
          self->num_connections++;
          log_pipe_append(&conn->super, &self->super.super.super);
        }
      else
        {
          log_pipe_unref(&conn->super);
          return FALSE;
        }
    }
  return TRUE;
}

static void
http_sd_accept(gpointer s)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;
  GSockAddr *peer_addr;
  gchar buf1[256], buf2[256];
  gint new_fd;
  gboolean res;
  int accepts = 0;

  while (accepts < MAX_ACCEPTS_AT_A_TIME)
    {
      GIOStatus status;

      status = g_accept(self->fd, &new_fd, &peer_addr);
      if (status == G_IO_STATUS_AGAIN)
        {
          /* no more connections to accept */
          break;
        }
      else if (status != G_IO_STATUS_NORMAL)
        {
          msg_error("Error accepting new connection",
                    evt_tag_error(EVT_TAG_OSERROR));
          return;
        }

      g_fd_set_nonblock(new_fd, TRUE);
      g_fd_set_cloexec(new_fd, TRUE);

      res = http_sd_process_connection(self, peer_addr, self->bind_addr, new_fd);

      if (res)
        {
          if (peer_addr->sa.sa_family != AF_UNIX)
            msg_notice("Connection accepted",
                       evt_tag_int("fd", new_fd),
                       evt_tag_str("client", g_sockaddr_format(peer_addr, buf1, sizeof(buf1), GSA_FULL)),
                       evt_tag_str("local", g_sockaddr_format(self->bind_addr, buf2, sizeof(buf2), GSA_FULL)));
          else
            msg_verbose("Connection accepted",
                        evt_tag_int("fd", new_fd),
                        evt_tag_str("client", g_sockaddr_format(peer_addr, buf1, sizeof(buf1), GSA_FULL)),
                        evt_tag_str("local", g_sockaddr_format(self->bind_addr, buf2, sizeof(buf2), GSA_FULL)));
        }
      else
        {
          close(new_fd);
        }

      g_sockaddr_unref(peer_addr);
      accepts++;
    }
  return;
}

static void
http_sd_close_connection(HTTPSourceDriver *self, HTTPSourceConnection *sc)
{
  gchar buf1[MAX_SOCKADDR_STRING], buf2[MAX_SOCKADDR_STRING];

  if (sc->peer_addr->sa.sa_family != AF_UNIX)
    msg_notice("Connection closed",
               evt_tag_int("fd", sc->sock),
               evt_tag_str("client", g_sockaddr_format(sc->peer_addr, buf1, sizeof(buf1), GSA_FULL)),
               evt_tag_str("local", g_sockaddr_format(self->bind_addr, buf2, sizeof(buf2), GSA_FULL)));
  else
    msg_verbose("Connection closed",
                evt_tag_int("fd", sc->sock),
                evt_tag_str("client", g_sockaddr_format(sc->peer_addr, buf1, sizeof(buf1), GSA_FULL)),
                evt_tag_str("local", g_sockaddr_format(self->bind_addr, buf2, sizeof(buf2), GSA_FULL)));

  log_reader_close_proto(sc->reader);
  log_pipe_deinit(&sc->super);
  self->connections = g_list_remove(self->connections, sc);
  http_sd_kill_connection(sc);
  self->num_connections--;
}

static void
http_sd_start_watches(HTTPSourceDriver *self)
{
  IV_FD_INIT(&self->listen_fd);
  self->listen_fd.fd = self->fd;
  self->listen_fd.cookie = self;
  self->listen_fd.handler_in = http_sd_accept;
  iv_fd_register(&self->listen_fd);
}

static void
http_sd_stop_watches(HTTPSourceDriver *self)
{
  if (iv_fd_registered(&self->listen_fd))
    iv_fd_unregister(&self->listen_fd);
}

static gboolean
http_sd_setup_reader_options(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  if (self->transport_mapper->sock_type == SOCK_STREAM && !self->window_size_initialized)
    {
      /* Distribute the window evenly between each of our possible
       * connections.  This is quite pessimistic and can result in very low
       * window sizes. Increase that but warn the user at the same time.
       */

      self->reader_options.super.init_window_size /= self->max_connections;
      if (self->reader_options.super.init_window_size < cfg->min_iw_size_per_reader)
        {
          msg_warning("WARNING: window sizing for tcp sources were changed in " VERSION_3_3
                      ", the configuration value was divided by the value of max-connections(). The result was too small, clamping to value of min_iw_size_per_reader. Ensure you have a proper log_fifo_size setting to avoid message loss.",
                      evt_tag_int("orig_log_iw_size", self->reader_options.super.init_window_size),
                      evt_tag_int("new_log_iw_size", cfg->min_iw_size_per_reader),
                      evt_tag_int("min_iw_size_per_reader", cfg->min_iw_size_per_reader),
                      evt_tag_int("min_log_fifo_size", cfg->min_iw_size_per_reader * self->max_connections));
          self->reader_options.super.init_window_size = cfg->min_iw_size_per_reader;
        }
      self->window_size_initialized = TRUE;
    }
  log_reader_options_init(&self->reader_options, cfg, self->super.super.group);
  return TRUE;
}

static gboolean
http_sd_setup_transport(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  if (!_transport_mapper_apply_transport(self->transport_mapper, cfg))
    return FALSE;

  http_sd_setup_reader_options(self);
  return TRUE;
}

static gboolean
http_sd_restore_kept_alive_connections(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  /* fetch persistent connections first */
  if (self->connections_kept_alive_across_reloads)
    {
      GList *p = NULL;
      self->connections = cfg_persist_config_fetch(cfg, http_sd_format_connections_name(self));

      self->num_connections = 0;
      for (p = self->connections; p; p = p->next)
        {
          http_sc_set_owner((HTTPSourceConnection *) p->data, self);
          if (log_pipe_init((LogPipe *) p->data))
            {
              self->num_connections++;
            }
          else
            {
              HTTPSourceConnection *sc = (HTTPSourceConnection *)p->data;

              self->connections = g_list_remove(self->connections, sc);
              http_sd_kill_connection((HTTPSourceConnection *)sc);
            }
        }
    }
  return TRUE;
}

static gboolean
_finalize_init(gpointer arg)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *)arg;
  /* set up listening source */
  if (listen(self->fd, self->listen_backlog) < 0)
    {
      msg_error("Error during listen()",
                evt_tag_error(EVT_TAG_OSERROR));
      close(self->fd);
      self->fd = -1;
      return FALSE;
    }

  http_sd_start_watches(self);
  char buf[256];
  msg_info("Accepting connections",
           evt_tag_str("addr", g_sockaddr_format(self->bind_addr, buf, sizeof(buf), GSA_FULL)));
  return TRUE;
}

static gboolean
_sd_open_stream(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);
  gint sock = -1;
  if (self->connections_kept_alive_across_reloads)
    {
      sock = GPOINTER_TO_UINT(cfg_persist_config_fetch(cfg, http_sd_format_listener_name(self))) - 1;
    }

  if (sock == -1)
    {
      if (!_transport_mapper_open_socket(self->transport_mapper, self->socket_options, self->bind_addr, _AFSOCKET_DIR_RECV,
                                         &sock))
        return self->super.super.optional;
    }
  self->fd = sock;
  return _transport_mapper_async_init(self->transport_mapper, _finalize_init, self);
}

static gboolean
http_sd_open_listener(HTTPSourceDriver *self)
{
  if (self->transport_mapper->sock_type == SOCK_STREAM)
    {
      return _sd_open_stream(self);
    }

  return FALSE;
}

static void
http_sd_close_fd(gpointer value)
{
  gint fd = GPOINTER_TO_UINT(value) - 1;
  close(fd);
}

static void
http_sd_save_connections(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  if (!self->connections_kept_alive_across_reloads || !cfg->persist)
    {
      http_sd_kill_connection_list(self->connections);
    }
  else
    {
      GList *p;

      for (p = self->connections; p; p = p->next)
        {
          log_pipe_deinit((LogPipe *) p->data);
        }
      cfg_persist_config_add(cfg, http_sd_format_connections_name(self), self->connections,
                             (GDestroyNotify)http_sd_kill_connection_list, FALSE);
    }
  self->connections = NULL;
}

static void
http_sd_save_listener(HTTPSourceDriver *self)
{
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super.super);

  if (self->transport_mapper->sock_type == SOCK_STREAM)
    {
      http_sd_stop_watches(self);
      if (!self->connections_kept_alive_across_reloads)
        {
          msg_verbose("Closing listener fd",
                      evt_tag_int("fd", self->fd));
          close(self->fd);
        }
      else
        {
          /* NOTE: the fd is incremented by one when added to persistent config
           * as persist config cannot store NULL */

          cfg_persist_config_add(cfg, http_sd_format_listener_name(self),
                                 GUINT_TO_POINTER(self->fd + 1), http_sd_close_fd, FALSE);
        }
    }
}

static gboolean
http_sd_setup_addresses(HTTPSourceDriver *s)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  g_sockaddr_unref(self->bind_addr);

  if (!resolve_hostname_to_sockaddr(&self->bind_addr, self->transport_mapper->address_family, self->bind_ip))
    return FALSE;

  if (!self->bind_port)
    {
      g_sockaddr_set_port(self->bind_addr, _transport_mapper_inet_get_server_port(self->transport_mapper));
    }
  else
    g_sockaddr_set_port(self->bind_addr, afinet_lookup_service(self->transport_mapper, self->bind_port));

  return TRUE;
}

gboolean
http_sd_init_method(LogPipe *s)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  return log_src_driver_init_method(s) &&
         http_sd_setup_transport(self) &&
         http_sd_setup_addresses(self) &&
         http_sd_restore_kept_alive_connections(self) &&
         http_sd_open_listener(self);
}

gboolean
http_sd_deinit_method(LogPipe *s)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  http_sd_save_connections(self);
  http_sd_save_listener(self);

  return log_src_driver_deinit_method(s);
}

static void
http_sd_notify(LogPipe *s, gint notify_code, gpointer user_data)
{
  switch (notify_code)
    {
    case NC_CLOSE:
    case NC_READ_ERROR:
    {
      g_assert_not_reached();
      break;
    }
    default:
      break;
    }
}

void
http_sd_free_method(LogPipe *s)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  log_reader_options_destroy(&self->reader_options);
  _transport_mapper_free(self->transport_mapper);
  _socket_options_free(self->socket_options);
  g_sockaddr_unref(self->bind_addr);
  self->bind_addr = NULL;

  g_free(self->bind_ip);
  g_free(self->bind_port);

  log_src_driver_free(s);
}

void
http_sd_set_localport(LogDriver *s, gchar *service)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  if (self->bind_port)
    g_free(self->bind_port);
  self->bind_port = g_strdup(service);
}

void
http_sd_set_localip(LogDriver *s, gchar *ip)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  if (self->bind_ip)
    g_free(self->bind_ip);
  self->bind_ip = g_strdup(ip);
}

void
http_sd_set_tls_context(LogDriver *s, TLSContext *tls_context)
{
  HTTPSourceDriver *self = (HTTPSourceDriver *) s;

  _transport_mapper_inet_set_tls_context((_TransportMapperInet *) self->transport_mapper, tls_context, NULL);
}

void
http_sd_init_instance(HTTPSourceDriver *self,
                      _SocketOptions *socket_options,
                      _TransportMapper *transport_mapper,
                      GlobalConfig *cfg)
{
  log_src_driver_init_instance(&self->super, cfg);

  self->super.super.super.init = http_sd_init_method;
  self->super.super.super.deinit = http_sd_deinit_method;
  self->super.super.super.free_fn = http_sd_free_method;
  self->super.super.super.notify = http_sd_notify;
  self->super.super.super.generate_persist_name = http_sd_format_name;
  self->socket_options = socket_options;
  self->transport_mapper = transport_mapper;
  self->max_connections = 10;
  self->listen_backlog = 255;
  self->connections_kept_alive_across_reloads = TRUE;
  log_reader_options_defaults(&self->reader_options);
  self->reader_options.super.stats_level = STATS_LEVEL1;
  self->reader_options.super.stats_source = transport_mapper->stats_source;

  self->reader_options.super.init_window_size = 1000;
}
