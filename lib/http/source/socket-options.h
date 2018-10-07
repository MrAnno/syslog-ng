#ifndef HTTP_SOCKET_OPTIONS_H_INCLUDED
#define HTTP_SOCKET_OPTIONS_H_INCLUDED

#include "gsockaddr.h"

typedef enum
{
  _AFSOCKET_DIR_RECV = 0x01,
  _AFSOCKET_DIR_SEND = 0x02,
} _AFSocketDirection;

typedef struct _TSocketOptions _SocketOptions;

struct _TSocketOptions
{
  /* socket options */
  gint so_sndbuf;
  gint so_rcvbuf;
  gint so_broadcast;
  gint so_keepalive;
  gboolean (*setup_socket)(_SocketOptions *s, gint sock, GSockAddr *bind_addr, _AFSocketDirection dir);
  void (*free)(gpointer s);
};

gboolean _socket_options_setup_socket_method(_SocketOptions *self, gint fd, GSockAddr *bind_addr,
                                             _AFSocketDirection dir);
void _socket_options_init_instance(_SocketOptions *self);
_SocketOptions *_socket_options_new(void);

static inline gboolean
_socket_options_setup_socket(_SocketOptions *s, gint sock, GSockAddr *bind_addr, _AFSocketDirection dir)
{
  return s->setup_socket(s, sock, bind_addr, dir);
}

static inline void
_socket_options_free(_SocketOptions *s)
{
  s->free(s);
}

#endif
