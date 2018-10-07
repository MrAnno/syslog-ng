#include "transport-mapper.h"
#include "gprocess.h"
#include "gsockaddr.h"
#include "gsocket.h"
#include "messages.h"
#include "fdhelpers.h"
#include "transport/transport-socket.h"

#include <errno.h>
#include <unistd.h>

static gboolean
_transport_mapper_privileged_bind(gint sock, GSockAddr *bind_addr)
{
  cap_t saved_caps;
  GIOStatus status;

  saved_caps = g_process_cap_save();
  g_process_enable_cap("cap_net_bind_service");
  g_process_enable_cap("cap_dac_override");

  status = g_bind(sock, bind_addr);

  g_process_cap_restore(saved_caps);
  return status == G_IO_STATUS_NORMAL;
}

gboolean
_transport_mapper_open_socket(_TransportMapper *self,
                              _SocketOptions *socket_options,
                              GSockAddr *bind_addr,
                              _AFSocketDirection dir,
                              int *fd)
{
  gint sock;

  sock = socket(self->address_family, self->sock_type, self->sock_proto);
  if (sock < 0)
    {
      msg_error("Error creating socket",
                evt_tag_error(EVT_TAG_OSERROR));
      goto error;
    }

  g_fd_set_nonblock(sock, TRUE);
  g_fd_set_cloexec(sock, TRUE);

  if (!_socket_options_setup_socket(socket_options, sock, bind_addr, dir))
    goto error_close;

  if (!_transport_mapper_privileged_bind(sock, bind_addr))
    {
      gchar buf[256];

      msg_error("Error binding socket",
                evt_tag_str("addr", g_sockaddr_format(bind_addr, buf, sizeof(buf), GSA_FULL)),
                evt_tag_error(EVT_TAG_OSERROR));
      goto error_close;
    }

  *fd = sock;
  return TRUE;

error_close:
  close(sock);
error:
  *fd = -1;
  return FALSE;
}

gboolean
_transport_mapper_apply_transport_method(_TransportMapper *self, GlobalConfig *cfg)
{
  return TRUE;
}

LogTransport *
_transport_mapper_construct_log_transport_method(_TransportMapper *self, gint fd)
{
  if (self->sock_type == SOCK_DGRAM)
    return log_transport_dgram_socket_new(fd);
  else
    return log_transport_stream_socket_new(fd);
}

void
_transport_mapper_set_transport(_TransportMapper *self, const gchar *transport)
{
  g_free(self->transport);
  self->transport = g_strdup(transport);
}

void
_transport_mapper_set_address_family(_TransportMapper *self, gint address_family)
{
  self->address_family = address_family;
}

void
_transport_mapper_free_method(_TransportMapper *self)
{
  g_free(self->transport);
}

void
_transport_mapper_init_instance(_TransportMapper *self, const gchar *transport)
{
  self->transport = g_strdup(transport);
  self->address_family = -1;
  self->sock_type = -1;
  self->free_fn = _transport_mapper_free_method;
  self->apply_transport = _transport_mapper_apply_transport_method;
  self->construct_log_transport = _transport_mapper_construct_log_transport_method;
}

void
_transport_mapper_free(_TransportMapper *self)
{
  if (self->free_fn)
    self->free_fn(self);

  g_free(self);
}
