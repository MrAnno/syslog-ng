#include "socket-options.h"
#include "messages.h"

#include <sys/types.h>
#include <sys/socket.h>

gboolean
_socket_options_setup_socket_method(_SocketOptions *self, gint fd, GSockAddr *bind_addr, _AFSocketDirection dir)
{

  gint rc;
  if (dir & _AFSOCKET_DIR_RECV)
    {
      if (self->so_rcvbuf)
        {
          gint so_rcvbuf_set = 0;
          socklen_t sz = sizeof(so_rcvbuf_set);

          rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &self->so_rcvbuf, sizeof(self->so_rcvbuf));
          if (rc < 0 ||
              getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf_set, &sz) < 0 ||
              sz != sizeof(so_rcvbuf_set) ||
              so_rcvbuf_set < self->so_rcvbuf)
            {
              msg_warning("The kernel refused to set the receive buffer (SO_RCVBUF) to the requested size, you probably need to adjust buffer related kernel parameters",
                          evt_tag_int("so_rcvbuf", self->so_rcvbuf),
                          evt_tag_int("so_rcvbuf_set", so_rcvbuf_set));
            }
        }
    }
  if (dir & _AFSOCKET_DIR_SEND)
    {
      if (self->so_sndbuf)
        {
          gint so_sndbuf_set = 0;
          socklen_t sz = sizeof(so_sndbuf_set);

          rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &self->so_sndbuf, sizeof(self->so_sndbuf));

          if (rc < 0 ||
              getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &so_sndbuf_set, &sz) < 0 ||
              sz != sizeof(so_sndbuf_set) ||
              so_sndbuf_set < self->so_sndbuf)
            {
              msg_warning("The kernel refused to set the send buffer (SO_SNDBUF) to the requested size, you probably need to adjust buffer related kernel parameters",
                          evt_tag_int("so_sndbuf", self->so_sndbuf),
                          evt_tag_int("so_sndbuf_set", so_sndbuf_set));
            }
        }
      if (self->so_broadcast)
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &self->so_broadcast, sizeof(self->so_broadcast));
    }
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &self->so_keepalive, sizeof(self->so_keepalive));
  return TRUE;
}

void
_socket_options_init_instance(_SocketOptions *self)
{
  self->setup_socket = _socket_options_setup_socket_method;
  self->free = g_free;
}

_SocketOptions *
_socket_options_new(void)
{
  _SocketOptions *self = g_new0(_SocketOptions, 1);

  _socket_options_init_instance(self);
  return self;
}
