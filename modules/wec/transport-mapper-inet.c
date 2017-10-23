/*
 * Copyright (c) 2002-2013 Balabit
 * Copyright (c) 1998-2013 Balázs Scheidler
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
#include "transport-mapper-inet.h"
#include "afinet.h"
#include "cfg.h"
#include "messages.h"
#include "stats/stats-registry.h"
#include "transport/transport-tls.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#define SYSLOG_TRANSPORT_TCP_PORT 601
#define SYSLOG_TRANSPORT_TLS_PORT 6514

static inline gboolean
_is_tls_required(TransportMapperInet *self)
{
  return self->require_tls;
}

static inline gboolean
_is_tls_allowed(TransportMapperInet *self)
{
  return self->require_tls || self->allow_tls;
}

static gboolean
transport_mapper_inet_validate_tls_options(TransportMapperInet *self)
{
  if (!self->tls_context && _is_tls_required(self))
    {
      msg_error("transport(tls) was specified, but tls() options missing");
      // evt_tag_str("id", self->super.super.super.id),
      return FALSE;
    }
  else if (self->tls_context && !_is_tls_allowed(self))
    {
      msg_error("tls() options specified for a transport that doesn't allow TLS encryption",
                //evt_tag_str("id", self->super.super.super.id),
                evt_tag_str("transport", self->super.transport));
      return FALSE;
    }
  return TRUE;
}

static gboolean
transport_mapper_inet_apply_transport_method(TransportMapper *s, GlobalConfig *cfg)
{
  TransportMapperInet *self = (TransportMapperInet *) s;

  if (!transport_mapper_apply_transport_method(s, cfg))
    return FALSE;

  return transport_mapper_inet_validate_tls_options(self);
}

static LogTransport *
transport_mapper_inet_construct_log_transport(TransportMapper *s, gint fd)
{
  TransportMapperInet *self = (TransportMapperInet *) s;

  if (self->tls_context)
    {
      TLSSession *tls_session;

      tls_session = tls_context_setup_session(self->tls_context);
      if (!tls_session)
        return NULL;

      tls_session_set_verify(tls_session, self->tls_verify_callback, self->tls_verify_data, NULL);
      return log_transport_tls_new(tls_session, fd);
    }
  else
    return transport_mapper_construct_log_transport_method(s, fd);
}

static gboolean
transport_mapper_inet_init(TransportMapper *s)
{
  TransportMapperInet *self = (TransportMapperInet *) s;

  if (self->tls_context && !tls_context_setup_context(self->tls_context))
    return FALSE;

  return TRUE;
}

void
transport_mapper_inet_free_method(TransportMapper *s)
{
  TransportMapperInet *self = (TransportMapperInet *) s;

  if (self->tls_context)
    tls_context_free(self->tls_context);
  transport_mapper_free_method(s);
}

void
transport_mapper_inet_init_instance(TransportMapperInet *self, const gchar *transport)
{
  transport_mapper_init_instance(&self->super, transport);
  self->super.apply_transport = transport_mapper_inet_apply_transport_method;
  self->super.construct_log_transport = transport_mapper_inet_construct_log_transport;
  self->super.init = transport_mapper_inet_init;
  self->super.free_fn = transport_mapper_inet_free_method;
  self->super.address_family = AF_INET;
}


TransportMapperInet *
transport_mapper_inet_new_instance(const gchar *transport)
{
  TransportMapperInet *self = g_new0(TransportMapperInet, 1);

  transport_mapper_inet_init_instance(self, transport);
  return self;
}

static gboolean
transport_mapper_syslog_apply_transport(TransportMapper *s, GlobalConfig *cfg)
{
  // TODO: remove, clean-up
  TransportMapperInet *self = (TransportMapperInet *) s;
  const gchar *transport = self->super.transport;

  if (!transport_mapper_apply_transport_method(s, cfg))
    return FALSE;

  if (strcasecmp(transport, "tls") == 0)
    {
      self->server_port = SYSLOG_TRANSPORT_TLS_PORT;
      self->super.logproto = "http";
      self->super.sock_type = SOCK_STREAM;
      self->super.sock_proto = IPPROTO_TCP;
      self->require_tls = TRUE;
    }
  else
    {
      self->server_port = SYSLOG_TRANSPORT_TCP_PORT;
      self->super.logproto = "http";
      self->super.sock_type = SOCK_STREAM;
      self->super.sock_proto = IPPROTO_TCP;
    }

  if (!transport_mapper_inet_validate_tls_options(self))
    return FALSE;

  return TRUE;
}

TransportMapper *
transport_mapper_syslog_new(void)
{
  TransportMapperInet *self = transport_mapper_inet_new_instance("tcp");

  self->super.apply_transport = transport_mapper_syslog_apply_transport;
  self->super.stats_source = SCS_SYSLOG;
  return &self->super;
}
