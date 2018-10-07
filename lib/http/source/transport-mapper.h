#ifndef HTTP_TRANSPORT_MAPPER_H_INCLUDED
#define HTTP_TRANSPORT_MAPPER_H_INCLUDED

#include "http/source/socket-options.h"
#include "transport/logtransport.h"
#include "gsockaddr.h"

typedef struct _TTransportMapper _TransportMapper;
typedef gboolean (*_TransportMapperAsyncInitCB)(gpointer arg);

struct _TTransportMapper
{
  /* the transport() option as specified by the user */
  gchar *transport;

  /* output parameters as determinted by _TransportMapper */
  gint address_family;
  /* SOCK_DGRAM or SOCK_STREAM or other SOCK_XXX values used by the socket() call */
  gint sock_type;
  /* protocol parameter for the socket() call, 0 for default or IPPROTO_XXX for specific transports */
  gint sock_proto;
  /* when a proto needs a Multitransport instance */
  gboolean create_multitransport;

  const gchar *logproto;
  gint stats_source;

  gboolean (*apply_transport)(_TransportMapper *self, GlobalConfig *cfg);
  LogTransport *(*construct_log_transport)(_TransportMapper *self, gint fd);
  gboolean (*init)(_TransportMapper *self);
  gboolean (*async_init)(_TransportMapper *self, _TransportMapperAsyncInitCB func, gpointer arg);
  void (*free_fn)(_TransportMapper *self);
};

void _transport_mapper_set_transport(_TransportMapper *self, const gchar *transport);
void _transport_mapper_set_address_family(_TransportMapper *self, gint address_family);

gboolean _transport_mapper_open_socket(_TransportMapper *self,
                                       _SocketOptions *socket_options,
                                       GSockAddr *bind_addr,
                                       _AFSocketDirection dir,
                                       int *fd);

gboolean _transport_mapper_apply_transport_method(_TransportMapper *self, GlobalConfig *cfg);
LogTransport *_transport_mapper_construct_log_transport_method(_TransportMapper *self, gint fd);

void _transport_mapper_init_instance(_TransportMapper *self, const gchar *transport);
void _transport_mapper_free(_TransportMapper *self);
void _transport_mapper_free_method(_TransportMapper *self);

static inline gboolean
_transport_mapper_apply_transport(_TransportMapper *self, GlobalConfig *cfg)
{
  return self->apply_transport(self, cfg);
}

static inline LogTransport *
_transport_mapper_construct_log_transport(_TransportMapper *self, gint fd)
{
  return self->construct_log_transport(self, fd);
}

static inline gboolean
_transport_mapper_init(_TransportMapper *self)
{
  if (self->init)
    return self->init(self);

  return TRUE;
}

static inline gboolean
_transport_mapper_async_init(_TransportMapper *self, _TransportMapperAsyncInitCB func, gpointer arg)
{
  if (self->async_init)
    {
      return self->async_init(self, func, arg);
    }

  return func(arg);
}
#endif
