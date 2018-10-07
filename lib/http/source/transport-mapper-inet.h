#ifndef HTTP_TRANSPORT_MAPPER_INET_H_INCLUDED
#define HTTP_TRANSPORT_MAPPER_INET_H_INCLUDED

#include "http/source/transport-mapper.h"
#include "tlscontext.h"

typedef struct _TTransportMapperInet
{
  _TransportMapper super;

  gint server_port;
  gboolean require_tls;
  gboolean allow_tls;
  gboolean require_tls_when_has_tls_context;
  TLSContext *tls_context;
  TLSVerifier *tls_verifier;
  gpointer secret_store_cb_data;
} _TransportMapperInet;

static inline gint
_transport_mapper_inet_get_server_port(const _TransportMapper *self)
{
  return ((_TransportMapperInet *) self)->server_port;
}

static inline void
_transport_mapper_inet_set_server_port(_TransportMapper *self, gint server_port)
{
  ((_TransportMapperInet *) self)->server_port = server_port;
}

static inline void
_transport_mapper_inet_set_tls_context(_TransportMapperInet *self, TLSContext *tls_context, TLSVerifier *tls_verifier)
{
  self->tls_context = tls_context;
  self->tls_verifier = tls_verifier;
}

void _transport_mapper_inet_init_instance(_TransportMapperInet *self, const gchar *transport);
_TransportMapper *_transport_mapper_network_new(void);

#endif
