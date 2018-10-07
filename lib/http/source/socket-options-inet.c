#include "socket-options-inet.h"
#include "gsockaddr.h"
#include "messages.h"

#include <string.h>
#include <netinet/tcp.h>

#ifndef SOL_IP
#define SOL_IP IPPROTO_IP
#endif

#ifndef SOL_IPV6
#define SOL_IPV6 IPPROTO_IPV6
#endif

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

static gboolean
_socket_options_inet_setup_socket(_SocketOptions *s, gint fd, GSockAddr *addr, _AFSocketDirection dir)
{
  _SocketOptionsInet *self = (_SocketOptionsInet *) s;
  gint off = 0;
  gint on = 1;

  if (!_socket_options_setup_socket_method(s, fd, addr, dir))
    return FALSE;

  if (self->tcp_keepalive_time > 0)
    {
#ifdef TCP_KEEPIDLE
      setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &self->tcp_keepalive_time, sizeof(self->tcp_keepalive_time));
#else
      msg_error("tcp-keepalive-time() is set but no TCP_KEEPIDLE setsockopt on this platform");
      return FALSE;
#endif
    }
  if (self->tcp_keepalive_probes > 0)
    {
#ifdef TCP_KEEPCNT
      setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &self->tcp_keepalive_probes, sizeof(self->tcp_keepalive_probes));
#else
      msg_error("tcp-keepalive-probes() is set but no TCP_KEEPCNT setsockopt on this platform");
      return FALSE;
#endif
    }
  if (self->tcp_keepalive_intvl > 0)
    {
#ifdef TCP_KEEPINTVL
      setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &self->tcp_keepalive_intvl, sizeof(self->tcp_keepalive_intvl));
#else
      msg_error("tcp-keepalive-intvl() is set but no TCP_KEEPINTVL setsockopt on this platform");
      return FALSE;
#endif
    }

  switch (addr->sa.sa_family)
    {
    case AF_INET:
    {
      struct ip_mreq mreq;

      if (IN_MULTICAST(ntohl(g_sockaddr_inet_get_address(addr).s_addr)))
        {
          if (dir & _AFSOCKET_DIR_RECV)
            {
              memset(&mreq, 0, sizeof(mreq));
              mreq.imr_multiaddr = g_sockaddr_inet_get_address(addr);
              mreq.imr_interface.s_addr = INADDR_ANY;
              setsockopt(fd, SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
              setsockopt(fd, SOL_IP, IP_MULTICAST_LOOP, &off, sizeof(off));
            }
          if (dir & _AFSOCKET_DIR_SEND)
            {
              if (self->ip_ttl)
                setsockopt(fd, SOL_IP, IP_MULTICAST_TTL, &self->ip_ttl, sizeof(self->ip_ttl));
            }

        }
      else
        {
          if (self->ip_ttl && (dir & _AFSOCKET_DIR_SEND))
            setsockopt(fd, SOL_IP, IP_TTL, &self->ip_ttl, sizeof(self->ip_ttl));
        }
      if (self->ip_tos && (dir & _AFSOCKET_DIR_SEND))
        setsockopt(fd, SOL_IP, IP_TOS, &self->ip_tos, sizeof(self->ip_tos));

      if (self->ip_freebind && (dir & _AFSOCKET_DIR_RECV))
        {
#ifdef IP_FREEBIND
          setsockopt(fd, SOL_IP, IP_FREEBIND, &on, sizeof(on));
#else
          msg_error("ip-freebind() is set but no IP_FREEBIND setsockopt on this platform");
          return FALSE;
#endif
        }
      break;
    }
#if SYSLOG_NG_ENABLE_IPV6
    case AF_INET6:
    {
      struct ipv6_mreq mreq6;

      if (IN6_IS_ADDR_MULTICAST(&g_sockaddr_inet6_get_sa(addr)->sin6_addr))
        {
          if (dir & _AFSOCKET_DIR_RECV)
            {
              memset(&mreq6, 0, sizeof(mreq6));
              mreq6.ipv6mr_multiaddr = *g_sockaddr_inet6_get_address(addr);
              mreq6.ipv6mr_interface = 0;
              setsockopt(fd, SOL_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6));
              setsockopt(fd, SOL_IPV6, IPV6_MULTICAST_LOOP, &off, sizeof(off));
            }
          if (dir & _AFSOCKET_DIR_SEND)
            {
              if (self->ip_ttl)
                setsockopt(fd, SOL_IPV6, IPV6_MULTICAST_HOPS, &self->ip_ttl, sizeof(self->ip_ttl));
            }
        }
      else
        {
          if (self->ip_ttl && (dir & _AFSOCKET_DIR_SEND))
            setsockopt(fd, SOL_IPV6, IPV6_UNICAST_HOPS, &self->ip_ttl, sizeof(self->ip_ttl));
        }

      if (self->ip_freebind && (dir & _AFSOCKET_DIR_RECV))
        {
#ifdef IP_FREEBIND
          /* NOTE: there's no separate IP_FREEBIND option for IPV6, it re-uses the IPv4 one */
          setsockopt(fd, SOL_IP, IP_FREEBIND, &on, sizeof(on));
#else
          msg_error("ip-freebind() is set but no IP_FREEBIND setsockopt on this platform");
          return FALSE;
#endif
        }
      break;
    }
#endif
    default:
      g_assert_not_reached();
      break;
    }
  return TRUE;
}

static _SocketOptionsInet *
_socket_options_inet_new_instance(void)
{
  _SocketOptionsInet *self = g_new0(_SocketOptionsInet, 1);

  _socket_options_init_instance(&self->super);
  self->super.setup_socket = _socket_options_inet_setup_socket;
  self->super.so_keepalive = TRUE;
#if defined(TCP_KEEPTIME) && defined(TCP_KEEPIDLE) && defined(TCP_KEEPCNT)
  self->tcp_keepalive_time = 60;
  self->tcp_keepalive_intvl = 10;
  self->tcp_keepalive_probes = 6;
#endif
  return self;
}

_SocketOptions *
_socket_options_inet_new(void)
{
  return &_socket_options_inet_new_instance()->super;
}
