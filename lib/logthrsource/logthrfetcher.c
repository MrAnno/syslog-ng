/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018-2019 László Várady <laszlo.varady@balabit.com>
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

#include "logthrfetcher.h"
#include "messages.h"

void
log_threaded_fetcher_set_fetch_no_data_delay(LogDriver *s, time_t no_data_delay)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) s;
  self->no_data_delay = no_data_delay;
}

static EVTTAG *
_tag_driver(LogThreadedFetcher *f)
{
  return evt_tag_str("driver", f->super.super.super.id);
}

static inline void
_thread_init(LogThreadedFetcher *self)
{
  msg_trace("Fetcher thread_init()", _tag_driver(self));
  if (self->thread_init)
    self->thread_init(self);
}

static inline void
_thread_deinit(LogThreadedFetcher *self)
{
  msg_trace("Fetcher thread_deinit()", _tag_driver(self));
  if (self->thread_deinit)
    self->thread_deinit(self);
}

static inline gboolean
_connect(LogThreadedFetcher *self)
{
  msg_trace("Fetcher connect()", _tag_driver(self));
  if (!self->connect)
    return TRUE;

  if (!self->connect(self))
    {
      msg_debug("Error establishing connection", _tag_driver(self));
      return FALSE;
    }

  return TRUE;
}

static inline void
_disconnect(LogThreadedFetcher *self)
{
  msg_trace("Fetcher disconnect()", _tag_driver(self));
  if (self->disconnect)
    self->disconnect(self);
}

static void
_start_reconnect_timer(LogThreadedFetcher *self)
{
  iv_validate_now();
  self->reconnect_timer.expires  = iv_now;
  self->reconnect_timer.expires.tv_sec += self->time_reopen;
  iv_timer_register(&self->reconnect_timer);
}

static void
_start_no_data_timer(LogThreadedFetcher *self)
{
  iv_validate_now();
  self->no_data_timer.expires  = iv_now;
  self->no_data_timer.expires.tv_sec += self->no_data_delay;
  iv_timer_register(&self->no_data_timer);
}

static void
_worker_run(LogThreadedSourceDriver *s)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) s;

  /* iv_init() and iv_deinit() are called by LogThreadedSourceDriver */

  iv_event_register(&self->wakeup_event);
  iv_event_register(&self->shutdown_event);

  _thread_init(self);
  if (_connect(self))
    iv_task_register(&self->fetch_task);
  else
    _start_reconnect_timer(self);

  iv_main();

  _disconnect(self);
  _thread_deinit(self);
}

static void
_worker_request_exit(LogThreadedSourceDriver *s)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) s;

  self->under_termination = TRUE;

  iv_event_post(&self->shutdown_event);

  if (self->request_exit)
    self->request_exit(self);
}

static void
_wakeup(LogThreadedSourceDriver *s)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) s;

  if (!self->under_termination)
    iv_event_post(&self->wakeup_event);
}

static inline void
_schedule_next_fetch_if_free_to_send(LogThreadedFetcher *self)
{
  if (log_threaded_source_free_to_send(&self->super))
    iv_task_register(&self->fetch_task);
  else
    self->suspended = TRUE;
}

static void
_on_fetch_error(LogThreadedFetcher *self)
{
  msg_error("Error during fetching messages", _tag_driver(self));
  _disconnect(self);
  _start_reconnect_timer(self);
}

static void
_on_not_connected(LogThreadedFetcher *self)
{
  msg_info("Fetcher disconnected while receiving messages, reconnecting", _tag_driver(self));
  _start_reconnect_timer(self);
}

static void
_on_fetch_success(LogThreadedFetcher *self, LogMessage *msg)
{
  log_threaded_source_post(&self->super, msg);
  _schedule_next_fetch_if_free_to_send(self);
}

static void
_on_fetch_try_again(LogThreadedFetcher *self)
{
  msg_debug("Try again when fetching messages", _tag_driver(self));
  iv_task_register(&self->fetch_task);
}

static void
_on_fetch_no_data(LogThreadedFetcher *self)
{
  msg_debug("No data during fetching messages", _tag_driver(self));
  _start_no_data_timer(self);
}


static void
_fetch(gpointer data)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) data;

  msg_trace("Fetcher fetch()", _tag_driver(self));

  LogThreadedFetchResult fetch_result = self->fetch(self);

  switch (fetch_result.result)
    {
    case THREADED_FETCH_ERROR:
      _on_fetch_error(self);
      break;

    case THREADED_FETCH_NOT_CONNECTED:
      _on_not_connected(self);
      break;

    case THREADED_FETCH_SUCCESS:
      _on_fetch_success(self, fetch_result.msg);
      break;

    case THREADED_FETCH_TRY_AGAIN:
      _on_fetch_try_again(self);
      break;

    case THREADED_FETCH_NO_DATA:
      _on_fetch_no_data(self);
      break;

    default:
      g_assert_not_reached();
    }
}

static void
_wakeup_event_handler(gpointer data)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) data;

  if (self->suspended && log_threaded_source_free_to_send(&self->super))
    {
      self->suspended = FALSE;

      if (!iv_task_registered(&self->fetch_task))
        iv_task_register(&self->fetch_task);
    }
}

static void
_stop_watches(LogThreadedFetcher *self)
{
  iv_event_unregister(&self->wakeup_event);
  iv_event_unregister(&self->shutdown_event);

  if (iv_task_registered(&self->fetch_task))
    iv_task_unregister(&self->fetch_task);

  if (iv_timer_registered(&self->reconnect_timer))
    iv_timer_unregister(&self->reconnect_timer);

  if (iv_timer_registered(&self->no_data_timer))
    iv_timer_unregister(&self->no_data_timer);
}

static void
_shutdown_event_handler(gpointer data)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) data;

  _stop_watches(self);

  iv_quit();
}

static void
_reconnect(gpointer data)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) data;

  if (_connect(self))
    _schedule_next_fetch_if_free_to_send(self);
  else
    _start_reconnect_timer(self);
}

static void
_no_data(gpointer data)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) data;

  iv_task_register(&self->fetch_task);
}

static void
_init_watches(LogThreadedFetcher *self)
{
  IV_TASK_INIT(&self->fetch_task);
  self->fetch_task.cookie = self;
  self->fetch_task.handler = _fetch;

  IV_EVENT_INIT(&self->wakeup_event);
  self->wakeup_event.cookie = self;
  self->wakeup_event.handler = _wakeup_event_handler;

  IV_EVENT_INIT(&self->shutdown_event);
  self->shutdown_event.cookie = self;
  self->shutdown_event.handler = _shutdown_event_handler;

  IV_TIMER_INIT(&self->reconnect_timer);
  self->reconnect_timer.cookie = self;
  self->reconnect_timer.handler = _reconnect;

  IV_TIMER_INIT(&self->no_data_timer);
  self->no_data_timer.cookie = self;
  self->no_data_timer.handler = _no_data;

}

gboolean
log_threaded_fetcher_init_method(LogPipe *s)
{
  LogThreadedFetcher *self = (LogThreadedFetcher *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  if (!log_threaded_source_driver_init_method(s))
    return FALSE;

  g_assert(self->fetch);

  if (cfg && self->time_reopen == -1)
    self->time_reopen = cfg->time_reopen;

  if (self->no_data_delay == -1)
    self->no_data_delay = cfg->time_reopen;

  return TRUE;
}

gboolean
log_threaded_fetcher_deinit_method(LogPipe *s)
{
  return log_threaded_source_driver_deinit_method(s);
}

void
log_threaded_fetcher_free_method(LogPipe *s)
{
  log_threaded_source_driver_free_method(s);
}

void
log_threaded_fetcher_init_instance(LogThreadedFetcher *self, GlobalConfig *cfg)
{
  log_threaded_source_driver_init_instance(&self->super, cfg);

  self->time_reopen = -1;
  self->no_data_delay = -1;

  _init_watches(self);

  log_threaded_source_set_wakeup_func(&self->super, _wakeup);
  self->super.worker->run = _worker_run;
  self->super.worker->request_exit = _worker_request_exit;

  self->super.super.super.super.init = log_threaded_fetcher_init_method;
  self->super.super.super.super.deinit = log_threaded_fetcher_deinit_method;
  self->super.super.super.super.free_fn = log_threaded_fetcher_free_method;
}
