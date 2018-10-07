#ifndef HTTP_SOCKET_OPTIONS_INET_H_INCLUDED
#define HTTP_SOCKET_OPTIONS_INET_H_INCLUDED

#include "http/source/socket-options.h"

typedef struct _TSocketOptionsInet
{
  _SocketOptions super;
  /* user settings */
  gint ip_ttl;
  gint ip_tos;
  gboolean ip_freebind;
  gint tcp_keepalive_time;
  gint tcp_keepalive_intvl;
  gint tcp_keepalive_probes;
} _SocketOptionsInet;

_SocketOptions *_socket_options_inet_new(void);

#endif
