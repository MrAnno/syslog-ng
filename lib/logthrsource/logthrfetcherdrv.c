/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady <laszlo.varady@balabit.com>
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

#include "logthrfetcherdrv.h"

static void
_worker_run(LogThreadedSourceDriver *s)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) s;

  iv_init();

  iv_event_register(&self->wakeup_event);
  iv_event_register(&self->shutdown_event);
  iv_task_register(&self->fetch_task);

  iv_main();
  iv_deinit();
}

static void
_worker_request_exit(LogThreadedSourceDriver *s)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) s;

  iv_event_post(&self->shutdown_event);
}

static void
_wakeup(LogThreadedSourceDriver *s)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) s;

  iv_event_post(&self->wakeup_event);
}

static void
_fetch(gpointer data)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) data;

  LogThreadedFetchResult fetch_result = self->fetch(self);

  // TODO error handling, connect, etc.
  log_threaded_source_post(&self->super, fetch_result.msg);

  if (log_threaded_source_free_to_send(&self->super))
    iv_task_register(&self->fetch_task);
}

static void
_wakeup_event_handler(gpointer data)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) data;

  if (!iv_task_registered(&self->fetch_task))
    iv_task_register(&self->fetch_task);
}

static void
_shutdown_event_handler(gpointer data)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) data;

  iv_event_unregister(&self->wakeup_event);
  iv_event_unregister(&self->shutdown_event);

  if (iv_task_registered(&self->fetch_task))
    iv_task_unregister(&self->fetch_task);

  iv_quit();
}

gboolean
log_threaded_fetcher_driver_init_method(LogPipe *s)
{
  LogThreadedFetcherDriver *self = (LogThreadedFetcherDriver *) s;

  if (!log_threaded_source_driver_init_method(s))
    return FALSE;

  g_assert(self->fetch);

  return TRUE;
}

gboolean
log_threaded_fetcher_driver_deinit_method(LogPipe *s)
{
  return log_threaded_source_driver_deinit_method(s);
}

void
log_threaded_fetcher_driver_free_method(LogPipe *s)
{
  log_threaded_source_driver_free_method(s);
}

void
log_threaded_fetcher_driver_init_instance(LogThreadedFetcherDriver *self, GlobalConfig *cfg)
{
  log_threaded_source_driver_init_instance(&self->super, cfg);

  IV_TASK_INIT(&self->fetch_task);
  self->fetch_task.cookie = self;
  self->fetch_task.handler = _fetch;

  IV_EVENT_INIT(&self->wakeup_event);
  self->wakeup_event.cookie = self;
  self->wakeup_event.handler = _wakeup_event_handler;

  IV_EVENT_INIT(&self->shutdown_event);
  self->shutdown_event.cookie = self;
  self->shutdown_event.handler = _shutdown_event_handler;

  log_threaded_source_driver_set_worker_run(&self->super, _worker_run);
  log_threaded_source_driver_set_worker_request_exit(&self->super, _worker_request_exit);
  log_threaded_source_set_wakeup(&self->super, _wakeup);

  self->super.super.super.super.init = log_threaded_fetcher_driver_init_method;
  self->super.super.super.super.deinit = log_threaded_fetcher_driver_deinit_method;
  self->super.super.super.super.free_fn = log_threaded_fetcher_driver_free_method;
}
