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

/* TODO: scratch-buffers GC call + reclaim  */

#include "logthrsourcedrv.h"
#include "mainloop-worker.h"

#include <iv.h>
#include <iv_event.h>

struct _LogThreadedSourceWorker
{
  LogSource super;
  struct iv_task fetcher_task;
  struct iv_event shutdown_event;
  WorkerOptions options;
};

static void _fetch(gpointer data);
static void _shutdown(gpointer data);

static LogPipe *
log_threaded_source_worker_logpipe(LogThreadedSourceWorker *self)
{
  return &self->super.super;
}

static void
log_threaded_source_worker_set_options(LogThreadedSourceWorker *self, LogPipe *control,
                                       LogThreadedSourceWorkerOptions *options,
                                       const gchar *stats_id, const gchar *stats_instance)
{
  /* Position tracking is disabled, should we support it? */
  log_source_set_options(&self->super, &options->super, stats_id, stats_instance, TRUE, FALSE, control->expr_node);

  /*log_pipe_unref(self->control);
  log_pipe_ref(control);*/
}

void
log_threaded_source_worker_options_defaults(LogThreadedSourceWorkerOptions *options)
{
  log_source_options_defaults(&options->super);
}

void
log_threaded_source_worker_options_init(LogThreadedSourceWorkerOptions *options, GlobalConfig *cfg,
                                        const gchar *group_name)
{
  log_source_options_init(&options->super, cfg, group_name);
}

void
log_threaded_source_worker_options_destroy(LogThreadedSourceWorkerOptions *options)
{
  log_source_options_destroy(&options->super);
}

static void
_init_fetcher_task(LogThreadedSourceWorker *worker)
{
  IV_TASK_INIT(&worker->fetcher_task);
  worker->fetcher_task.cookie = worker;
  worker->fetcher_task.handler = _fetch;
}

static void
_schedule_fetcher_task(LogThreadedSourceWorker *worker)
{
  if (!iv_task_registered(&worker->fetcher_task))
    iv_task_register(&worker->fetcher_task);
}

static void
_deregister_fetcher_task(LogThreadedSourceWorker *worker)
{
  if (iv_task_registered(&worker->fetcher_task))
    iv_task_unregister(&worker->fetcher_task);
}

static void
_init_shutdown_event(LogThreadedSourceWorker *worker)
{
  IV_EVENT_INIT(&worker->shutdown_event);
  worker->shutdown_event.cookie = worker;
  worker->shutdown_event.handler = _shutdown;
}

static void
_fetch(gpointer data)
{
  LogThreadedSourceWorker *self = (LogThreadedSourceWorker *) data;
}

static void
_shutdown(gpointer data)
{
  LogThreadedSourceWorker *self = (LogThreadedSourceWorker *) data;

  _deregister_fetcher_task(self);
  iv_event_unregister(&self->shutdown_event);
  iv_quit();
}

static void
_worker_thread(gpointer data)
{
  LogThreadedSourceWorker *self = (LogThreadedSourceWorker *) data;

  iv_init();

  iv_event_register(&self->shutdown_event);
  _schedule_fetcher_task(self);

  iv_main();
  iv_deinit();
}

static void
_worker_schedule_exit(gpointer data)
{
  LogThreadedSourceWorker *self = (LogThreadedSourceWorker *) data;

  iv_event_post(&self->shutdown_event);
}

static gboolean
log_threaded_source_worker_init(LogPipe *s)
{
  LogThreadedSourceWorker *self = (LogThreadedSourceWorker *) s;
  if (!log_source_init(s))
    return FALSE;

  main_loop_create_worker_thread(_worker_thread, _worker_schedule_exit, self, &self->options);

  return TRUE;
}

static LogThreadedSourceWorker *
log_threaded_source_worker_new(GlobalConfig *cfg)
{
  LogThreadedSourceWorker *self = g_new0(LogThreadedSourceWorker, 1);
  log_source_init_instance(&self->super, cfg);

  _init_fetcher_task(self);
  _init_shutdown_event(self);

  self->options.is_external_input = TRUE;

  self->super.super.init = log_threaded_source_worker_init;
  /*self->super.super.deinit =
  self->super.super.free_fn =
  self->super.wakeup =
  self->super.window_empty_cb =*/

  return self;
}


gboolean
log_threaded_source_driver_init_method(LogPipe *s)
{
  LogThreadedSourceDriver *self = (LogThreadedSourceDriver *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  if (!log_src_driver_init_method(s))
    return FALSE;

  self->worker = log_threaded_source_worker_new(cfg);

  g_assert(self->format_stats_instance);

  log_threaded_source_worker_options_init(&self->worker_options, cfg, self->super.super.group);
  log_threaded_source_worker_set_options(self->worker, s, &self->worker_options,
                                         self->super.super.id, self->format_stats_instance(self));
  // state

  LogPipe *worker_pipe = log_threaded_source_worker_logpipe(self->worker);
  log_pipe_append(worker_pipe, s);
  if (!log_pipe_init(worker_pipe))
    {
      log_pipe_unref(worker_pipe);
      self->worker = NULL;
      return FALSE;
    }

  return TRUE;
}

gboolean
log_threaded_source_driver_deinit_method(LogPipe *s)
{
  LogThreadedSourceDriver *self = (LogThreadedSourceDriver *) s;
  LogPipe *worker_pipe = log_threaded_source_worker_logpipe(self->worker);

  log_pipe_deinit(worker_pipe);
  log_pipe_unref(worker_pipe);
  self->worker = NULL;

  return log_src_driver_deinit_method(s);
}

void
log_threaded_source_driver_free_method(LogPipe *s)
{
  LogThreadedSourceDriver *self = (LogThreadedSourceDriver *) s;

  log_threaded_source_worker_options_destroy(&self->worker_options);
  log_src_driver_free(s);
}

void
log_threaded_source_driver_init_instance(LogThreadedSourceDriver *self, GlobalConfig *cfg)
{
  log_src_driver_init_instance(&self->super, cfg);

  log_threaded_source_worker_options_defaults(&self->worker_options);

  self->super.super.super.init = log_threaded_source_driver_init_method;
  self->super.super.super.deinit = log_threaded_source_driver_deinit_method;
  self->super.super.super.free_fn = log_threaded_source_driver_free_method;
  // self->super.super.super.queue =
  // self->super.super.super.notify =
}
