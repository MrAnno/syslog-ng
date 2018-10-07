#include "transport-mapper-inet.h"
#include "cfg.h"
#include "messages.h"
#include "stats/stats-registry.h"
#include "transport/transport-tls.h"
#include "transport/multitransport.h"
#include "transport/transport-factory-tls.h"
#include "transport/transport-factory-socket.h"
#include "secret-storage/secret-storage.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#define HTTP_PORT 80
#define HTTPS_PORT 443

static inline gboolean
_is_tls_required(_TransportMapperInet *self)
{
  return self->require_tls || (self->tls_context && self->require_tls_when_has_tls_context);
}

static inline gboolean
_is_tls_allowed(_TransportMapperInet *self)
{
  return self->require_tls || self->allow_tls || self->require_tls_when_has_tls_context;
}

static gboolean
_transport_mapper_inet_validate_tls_options(_TransportMapperInet *self)
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
_transport_mapper_inet_apply_transport_method(_TransportMapper *s, GlobalConfig *cfg)
{
  _TransportMapperInet *self = (_TransportMapperInet *) s;

  if (!_transport_mapper_apply_transport_method(s, cfg))
    return FALSE;

  return _transport_mapper_inet_validate_tls_options(self);
}

static LogTransport *
_construct_multitransport_with_tls_factory(_TransportMapperInet *self, gint fd)
{
  TransportFactory *default_factory = transport_factory_tls_new(self->tls_context, self->tls_verifier);
  return multitransport_new(default_factory, fd);
}

static LogTransport *
_construct_tls_transport(_TransportMapperInet *self, gint fd)
{
  if (self->super.create_multitransport)
    return _construct_multitransport_with_tls_factory(self, fd);

  TLSSession *tls_session = tls_context_setup_session(self->tls_context);
  if (!tls_session)
    return NULL;

  tls_session_set_verifier(tls_session, self->tls_verifier);

  return log_transport_tls_new(tls_session, fd);
}

static LogTransport *
_construct_multitransport_with_plain_tcp_factory(_TransportMapperInet *self, gint fd)
{
  TransportFactory *default_factory = transport_factory_socket_new(self->super.sock_type);

  return multitransport_new(default_factory, fd);
}

static LogTransport *
_construct_multitransport_with_plain_and_tls_factories(_TransportMapperInet *self, gint fd)
{
  LogTransport *transport = _construct_multitransport_with_plain_tcp_factory(self, fd);

  TransportFactory *tls_factory = transport_factory_tls_new(self->tls_context, self->tls_verifier);
  multitransport_add_factory((MultiTransport *)transport, tls_factory);

  return transport;
}

static LogTransport *
_construct_plain_tcp_transport(_TransportMapperInet *self, gint fd)
{
  if (self->super.create_multitransport)
    return _construct_multitransport_with_plain_tcp_factory(self, fd);

  return _transport_mapper_construct_log_transport_method(&self->super, fd);
}

static LogTransport *
_transport_mapper_inet_construct_log_transport(_TransportMapper *s, gint fd)
{
  _TransportMapperInet *self = (_TransportMapperInet *) s;

  if (self->tls_context && _is_tls_required(self))
    {
      return _construct_tls_transport(self, fd);
    }

  if (self->tls_context)
    {
      return _construct_multitransport_with_plain_and_tls_factories(self, fd);
    }

  return _construct_plain_tcp_transport(self, fd);
}

static gboolean
_transport_mapper_inet_init(_TransportMapper *s)
{
  _TransportMapperInet *self = (_TransportMapperInet *) s;

  if (self->tls_context && (tls_context_setup_context(self->tls_context) != TLS_CONTEXT_SETUP_OK))
    return FALSE;

  return TRUE;
}

typedef struct _call_finalize_init_args
{
  _TransportMapperInet *transport_mapper_inet;
  _TransportMapperAsyncInitCB func;
  gpointer func_args;
} call_finalize_init_args;

static void
_call_finalize_init(Secret *secret, gpointer user_data)
{
  call_finalize_init_args *args = user_data;
  _TransportMapperInet *self = args->transport_mapper_inet;

  if (!self)
    return;

  TLSContextSetupResult r = tls_context_setup_context(self->tls_context);
  const gchar *key = tls_context_get_key_file(self->tls_context);

  switch (r)
    {
    case TLS_CONTEXT_SETUP_ERROR:
    {
      msg_error("Error setting up TLS context",
                evt_tag_str("keyfile", key));
      secret_storage_update_status(key, SECRET_STORAGE_STATUS_FAILED);
      return;
    }
    case TLS_CONTEXT_SETUP_BAD_PASSWORD:
    {
      msg_error("Invalid password, error setting up TLS context",
                evt_tag_str("keyfile", key));

      if (!secret_storage_subscribe_for_key(key, _call_finalize_init, args))
        msg_error("Failed to subscribe for key",
                  evt_tag_str("keyfile", key));
      else
        msg_debug("Re-subscribe for key",
                  evt_tag_str("keyfile", key));

      secret_storage_update_status(key, SECRET_STORAGE_STATUS_INVALID_PASSWORD);

      return;
    }
    default:
      secret_storage_update_status(key, SECRET_STORAGE_SUCCESS);
      if (!args->func(args->func_args))
        {
          msg_error("Error finalize initialization",
                    evt_tag_str("keyfile", key));
        }
    }
}

static gboolean
_transport_mapper_inet_async_init(_TransportMapper *s, _TransportMapperAsyncInitCB func, gpointer func_args)
{
  _TransportMapperInet *self = (_TransportMapperInet *)s;

  if (!self->tls_context)
    return func(func_args);

  TLSContextSetupResult tls_ctx_setup_res = tls_context_setup_context(self->tls_context);

  const gchar *key = tls_context_get_key_file(self->tls_context);

  if (tls_ctx_setup_res == TLS_CONTEXT_SETUP_OK)
    {
      if (key && secret_storage_contains_key(key))
        secret_storage_update_status(key, SECRET_STORAGE_SUCCESS);
      return func(func_args);
    }

  if (tls_ctx_setup_res == TLS_CONTEXT_SETUP_BAD_PASSWORD)
    {
      msg_error("Error setting up TLS context",
                evt_tag_str("keyfile", key));
      call_finalize_init_args *args = g_new0(call_finalize_init_args, 1);
      args->transport_mapper_inet = self;
      args->func = func;
      args->func_args = func_args;
      self->secret_store_cb_data = args;
      gboolean subscribe_res = secret_storage_subscribe_for_key(key, _call_finalize_init, args);
      if (subscribe_res)
        msg_info("Waiting for password",
                 evt_tag_str("keyfile", key));
      else
        msg_error("Failed to subscribe for key",
                  evt_tag_str("keyfile", key));
      return subscribe_res;
    }

  return FALSE;
}

void
_transport_mapper_inet_free_method(_TransportMapper *s)
{
  _TransportMapperInet *self = (_TransportMapperInet *) s;

  if (self->secret_store_cb_data)
    {
      const gchar *key = tls_context_get_key_file(self->tls_context);
      secret_storage_unsubscribe(key, _call_finalize_init, self->secret_store_cb_data);
      g_free(self->secret_store_cb_data);
    }

  if (self->tls_verifier)
    tls_verifier_unref(self->tls_verifier);
  if (self->tls_context)
    tls_context_unref(self->tls_context);

  _transport_mapper_free_method(s);
}

void
_transport_mapper_inet_init_instance(_TransportMapperInet *self, const gchar *transport)
{
  _transport_mapper_init_instance(&self->super, transport);
  self->super.apply_transport = _transport_mapper_inet_apply_transport_method;
  self->super.construct_log_transport = _transport_mapper_inet_construct_log_transport;
  self->super.init = _transport_mapper_inet_init;
  self->super.async_init = _transport_mapper_inet_async_init;
  self->super.free_fn = _transport_mapper_inet_free_method;
  self->super.address_family = AF_INET;
}


_TransportMapperInet *
_transport_mapper_inet_new_instance(const gchar *transport)
{
  _TransportMapperInet *self = g_new0(_TransportMapperInet, 1);

  _transport_mapper_inet_init_instance(self, transport);
  return self;
}

static gboolean
_transport_mapper_network_apply_transport(_TransportMapper *s, GlobalConfig *cfg)
{
  _TransportMapperInet *self = (_TransportMapperInet *) s;
  const gchar *transport;

  if (!_transport_mapper_apply_transport_method(s, cfg))
    return FALSE;

  transport = self->super.transport;
  if (strcasecmp(transport, "tls") == 0)
    {
      self->server_port = HTTPS_PORT;
      self->super.logproto = "text";
      self->super.sock_type = SOCK_STREAM;
      self->super.sock_proto = IPPROTO_TCP;
      self->require_tls = TRUE;
    }
  else if (strcasecmp(transport, "tcp") == 0)
    {
      self->server_port = HTTP_PORT;
      self->super.logproto = "text";
      self->super.sock_type = SOCK_STREAM;
      self->super.sock_proto = IPPROTO_TCP;
    }
  else
    {
      self->super.logproto = self->super.transport;
      self->super.sock_type = SOCK_STREAM;
      self->super.sock_proto = IPPROTO_TCP;
      self->server_port = HTTP_PORT;
      self->allow_tls = TRUE;
    }

  g_assert(self->server_port != 0);

  if (!_transport_mapper_inet_validate_tls_options(self))
    return FALSE;

  return TRUE;
}

_TransportMapper *
_transport_mapper_network_new(void)
{
  _TransportMapperInet *self = _transport_mapper_inet_new_instance("tcp");

  self->super.apply_transport = _transport_mapper_network_apply_transport;
  self->super.stats_source = SCS_NETWORK;
  return &self->super;
}
