/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady <laszlo.varady@balabit.com>
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

#ifndef HTTP_SOURCE_H
#define HTTP_SOURCE_H

#include "syslog-ng.h"
#include "http/source/http-source.h"

typedef struct EHTTPSourceDriver EHTTPSourceDriver;
typedef enum _EHTTPSourceMode
{
  EHTTP_SINGLE,
  EHTTP_TEXT,
  EHTTP_JSON
} EHTTPSourceMode;

struct EHTTPSourceDriver
{
  HTTPSourceDriver super;
  EHTTPSourceMode mode;
};

EHTTPSourceDriver *ehttp_sd_new(GlobalConfig *cfg);
gboolean ehttp_sd_set_mode(LogDriver *d, const gchar *mode);

#endif
