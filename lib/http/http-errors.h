/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 László Várady
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

/* TODO: configurable error responses */

#define HTTP_ERROR_500_PAGE \
  "HTTP/1.1 500 Internal Server Error\r\n" \
  "Server: syslog-ng\r\n" \
  "Content-Type: text/html\r\n" \
  "Content-Length: 160\r\n" \
  "Connection: close\r\n\r\n" \
  "<html><head><title>500 Internal Server Error</title></head>" \
  "<body><center><h1>500 Internal Server Error</h1></center>" \
  "<hr><center>syslog-ng</center></body></html>"
