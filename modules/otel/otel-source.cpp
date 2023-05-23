/*
 * Copyright (c) 2023 Attila Szakacs
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

#include <unistd.h>

#include "otel-source.h"
#include "otel-source.hpp"

#define get_OtelSourceDriverCpp(s) (((OtelSourceDriver *) s)->cpp)

/* C++ Implementations */

OtelSourceDriverCpp::OtelSourceDriverCpp(OtelSourceDriver *s)
  : super(s)
{
}

void
OtelSourceDriverCpp::run()
{
  while (!g_atomic_counter_get(&exit_requested))
    {
      LogMessage *msg = log_msg_new_empty();
      log_msg_set_value(msg, LM_V_MESSAGE, "Hello from OpenTelemetry C++ source", -1);

      log_threaded_source_blocking_post(&super->super, msg);

      usleep(1000000);
    }
}

void
OtelSourceDriverCpp::request_exit()
{
  g_atomic_counter_set(&exit_requested, TRUE);
}

const gchar *
OtelSourceDriverCpp::format_stats_instance()
{
  static gchar persist_name[1024];

  if (super->super.super.super.super.persist_name)
    g_snprintf(persist_name, sizeof(persist_name), "opentelemetry,%s",
               super->super.super.super.super.persist_name);
  else
    g_snprintf(persist_name, sizeof(persist_name), "opentelemetry,%lu",
               port);

  return persist_name;
}

gboolean
OtelSourceDriverCpp::init()
{
  return log_threaded_source_driver_init_method(&super->super.super.super.super);
}

gboolean
OtelSourceDriverCpp::deinit()
{
  return log_threaded_source_driver_deinit_method(&super->super.super.super.super);
}

/* Config setters */

void
otel_sd_set_port(LogDriver *s, guint64 port)
{
  get_OtelSourceDriverCpp(s)->port = port;
}

/* C Wrappers */

static void
_run(LogThreadedSourceDriver *s)
{
  get_OtelSourceDriverCpp(s)->run();
}

static void
_request_exit(LogThreadedSourceDriver *s)
{
  get_OtelSourceDriverCpp(s)->request_exit();
}

static const gchar *
_format_stats_instance(LogThreadedSourceDriver *s)
{
  return get_OtelSourceDriverCpp(s)->format_stats_instance();
}

static gboolean
_init(LogPipe *s)
{
  return get_OtelSourceDriverCpp(s)->init();
}

static gboolean
_deinit(LogPipe *s)
{
  return get_OtelSourceDriverCpp(s)->deinit();
}

static void
_free(LogPipe *s)
{
  delete get_OtelSourceDriverCpp(s);
  log_threaded_source_driver_free_method(s);
}

LogDriver *
otel_sd_new(GlobalConfig *cfg)
{
  OtelSourceDriver *s = g_new0(OtelSourceDriver, 1);
  log_threaded_source_driver_init_instance(&s->super, cfg);

  s->cpp = new OtelSourceDriverCpp(s);

  s->super.super.super.super.init = _init;
  s->super.super.super.super.deinit = _deinit;
  s->super.super.super.super.free_fn = _free;

  s->super.format_stats_instance = _format_stats_instance;
  s->super.run = _run;
  s->super.request_exit = _request_exit;

  return &s->super.super.super;
}
