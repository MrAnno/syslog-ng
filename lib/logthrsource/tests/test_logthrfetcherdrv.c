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

#include <criterion/criterion.h>

#include "logthrsource/logthrfetcher.h"
#include "apphook.h"
#include "mainloop.h"
#include "mainloop-worker.h"
#include "cfg.h"
#include "stats/stats-counter.h"
#include "logsource.h"
#include "cr_template.h"

typedef struct
{
  gint time_reopen;
  gint no_data_delay;
  gboolean (*connect)(LogThreadedFetcher *self);
  LogThreadedFetchResult (*fetch)(LogThreadedFetcher *self);
} Parameters;

Parameters parameters;

typedef struct
{
  LogThreadedSourceDriver super;

  gint num_of_messages_to_generate;
  gint num_of_connection_failures_to_generate;
  gint connect_counter;
  gboolean try_again_first_time;
  gboolean no_data_first_time;

} TestThreadedSourceDriver;

typedef struct _TestThreadedFetcher
{
  LogThreadedFetcher super;

  GMutex *lock;
  GCond *cond;

} TestThreadedFetcher;

static LogThreadedFetchResult _fetch(LogThreadedFetcher *s);

MainLoopOptions main_loop_options = {0};
MainLoop *main_loop;

static const gchar *
_generate_persist_name(const LogPipe *s)
{
  return "test_threaded_fetcher_driver";
}

static const gchar *
_format_stats_instance(LogThreadedSourceDriver *s)
{
  return "test_threaded_fetcher_driver_stats";
}

static void _source_queue_mock(LogPipe *s, LogMessage *msg, const LogPathOptions *path_options)
{
  LogSource *self = (LogSource *) s;

  stats_counter_inc(self->recvd_messages);
  log_pipe_forward_msg(s, msg, path_options);
}

static void
test_threaded_fetcher_free(LogPipe *s)
{
  TestThreadedFetcher *self = (TestThreadedFetcher *) s;

  g_cond_free(self->cond);
  g_mutex_free(self->lock);

  log_threaded_fetcher_free_method(s);
}

static LogThreadedSourceWorker *
create_threaded_fetcher(LogThreadedSourceDriver *drv)
{
  GlobalConfig *cfg = log_pipe_get_config(&drv->super.super.super);
  TestThreadedFetcher *self = g_new0(TestThreadedFetcher, 1);
  log_threaded_fetcher_init_instance(&self->super, cfg);

  /* mock out the hard-coded DNS lookup calls inside log_source_queue() */
  self->super.super.super.super.queue = _source_queue_mock;
  self->super.super.super.super.free_fn = test_threaded_fetcher_free;

  self->super.time_reopen = parameters.time_reopen;
  self->super.no_data_delay = parameters.no_data_delay;
  if (parameters.fetch)
    self->super.fetch = parameters.fetch;
  if (parameters.connect)
    self->super.connect = parameters.connect;

  self->lock = g_mutex_new();
  self->cond = g_cond_new();

  return &self->super.super;
}

static TestThreadedSourceDriver *
test_threaded_source_driver_new(GlobalConfig *cfg)
{
  TestThreadedSourceDriver *self = g_new0(TestThreadedSourceDriver, 1);

  log_threaded_source_driver_init_instance(&self->super, cfg);

  self->super.format_stats_instance = _format_stats_instance;
  self->super.super.super.super.generate_persist_name = _generate_persist_name;
  self->super.construct_worker = create_threaded_fetcher;

  return self;
}

static void
start_test_threaded_source_driver(TestThreadedSourceDriver *s)
{
  cr_assert(log_pipe_init(&s->super.super.super.super));
  app_config_changed();
}

static void
wait_for_messages(TestThreadedSourceDriver *s)
{
  TestThreadedFetcher *self = (TestThreadedFetcher *)s->super.worker;

  g_mutex_lock(self->lock);
  while (s->num_of_messages_to_generate > 0)
    g_cond_wait(self->cond, self->lock);
  g_mutex_unlock(self->lock);
}

static void
stop_test_threaded_source_driver(TestThreadedSourceDriver *s)
{
  main_loop_sync_worker_startup_and_teardown();
}

static void
destroy_test_threaded_source_driver(TestThreadedSourceDriver *s)
{
  cr_assert(log_pipe_deinit(&s->super.super.super.super));
  log_pipe_unref(&s->super.super.super.super);
}

static void
setup(void)
{
  app_startup();
  main_loop = main_loop_get_instance();
  main_loop_init(main_loop, &main_loop_options);
}

static void
teardown(void)
{
  main_loop_deinit(main_loop);
  app_shutdown();
}

static LogThreadedFetchResult
_fetch(LogThreadedFetcher *s)
{
  TestThreadedFetcher *self = (TestThreadedFetcher *) s;
  TestThreadedSourceDriver *drv = (TestThreadedSourceDriver *)s->super.owner;


  if (drv->num_of_connection_failures_to_generate
      && drv->connect_counter <= drv->num_of_connection_failures_to_generate)
    {
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_NOT_CONNECTED, NULL
      };
    }

  g_mutex_lock(self->lock);
  if (drv->num_of_messages_to_generate <= 0)
    {
      g_cond_signal(self->cond);
      g_mutex_unlock(self->lock);
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_ERROR, NULL
      };
    }

  LogMessage *msg = create_sample_message();

  drv->num_of_messages_to_generate--;
  g_mutex_unlock(self->lock);

  return (LogThreadedFetchResult)
  {
    .result = THREADED_FETCH_SUCCESS,
    .msg = msg
  };
}

static gboolean
_connect_fail_first_time(LogThreadedFetcher *s)
{
  TestThreadedSourceDriver *self = (TestThreadedSourceDriver *) s->super.owner;

  self->connect_counter++;
  if (self->connect_counter == 1)
    return FALSE;

  return TRUE;
}

TestSuite(logthrfetcherdrv, .init = setup, .fini = teardown, .timeout = 10);

Test(logthrfetcherdrv, test_simple_fetch)
{
  TestThreadedSourceDriver *s = test_threaded_source_driver_new(main_loop_get_current_config(main_loop));

  s->num_of_messages_to_generate = 10;
  parameters.fetch = _fetch;
  parameters.connect = _connect_fail_first_time;

  start_test_threaded_source_driver(s);
  wait_for_messages(s);
  stop_test_threaded_source_driver(s);

  TestThreadedFetcher *fetcher = (TestThreadedFetcher *)s->super.worker;
  StatsCounterItem *recvd_messages = fetcher->super.super.super.recvd_messages;
  cr_assert(stats_counter_get(recvd_messages) == 10);

  destroy_test_threaded_source_driver(s);
}

Test(logthrfetcherdrv, test_reconnect)
{
  TestThreadedSourceDriver *s = test_threaded_source_driver_new(main_loop_get_current_config(main_loop));

  s->num_of_messages_to_generate = 10;
  s->num_of_connection_failures_to_generate = 5;

  parameters.time_reopen = 0; /* immediate */
  parameters.connect = _connect_fail_first_time;
  parameters.fetch = _fetch;

  start_test_threaded_source_driver(s);
  wait_for_messages(s);
  stop_test_threaded_source_driver(s);

  TestThreadedFetcher *fetcher = (TestThreadedFetcher *)s->super.worker;
  StatsCounterItem *recvd_messages = fetcher->super.super.super.recvd_messages;
  cr_assert(stats_counter_get(recvd_messages) == 10);
  cr_assert_geq(s->connect_counter, 6);

  destroy_test_threaded_source_driver(s);
}

static LogThreadedFetchResult
_fetch_for_try_again_test(LogThreadedFetcher *s)
{
  TestThreadedSourceDriver *driver = (TestThreadedSourceDriver *) s->super.owner;
  TestThreadedFetcher *self = (TestThreadedFetcher *) s;

  if (driver->try_again_first_time)
    {
      driver->try_again_first_time = FALSE;
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_TRY_AGAIN, NULL
      };
    }

  g_mutex_lock(self->lock);
  if (driver->num_of_messages_to_generate <= 0)
    {
      g_cond_signal(self->cond);
      g_mutex_unlock(self->lock);
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_ERROR, NULL
      };
    }

  LogMessage *msg = create_sample_message();

  driver->num_of_messages_to_generate--;
  g_mutex_unlock(self->lock);

  return (LogThreadedFetchResult)
  {
    .result = THREADED_FETCH_SUCCESS,
    .msg = msg
  };
}

Test(logthrfetcherdrv, test_try_again)
{
  TestThreadedSourceDriver *s = test_threaded_source_driver_new(main_loop_get_current_config(main_loop));
  s->try_again_first_time = TRUE;

  s->num_of_messages_to_generate = 1;
  parameters.time_reopen = 10;
  parameters.fetch = _fetch_for_try_again_test;

  struct timespec start = {0};
  cr_assert(!clock_gettime(CLOCK_MONOTONIC, &start));

  start_test_threaded_source_driver(s);
  wait_for_messages(s);
  stop_test_threaded_source_driver(s);

  struct timespec stop = {0};
  cr_assert(!clock_gettime(CLOCK_MONOTONIC, &stop));

  // Should not pass time_reopen in case of try_again
  cr_assert(!stop.tv_sec - start.tv_sec < 2);

  destroy_test_threaded_source_driver(s);
}

static LogThreadedFetchResult
_fetch_for_no_data(LogThreadedFetcher *s)
{
  TestThreadedSourceDriver *driver = (TestThreadedSourceDriver *) s->super.owner;
  TestThreadedFetcher *self = (TestThreadedFetcher *) s;

  if (driver->no_data_first_time)
    {
      driver->no_data_first_time = FALSE;
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_NO_DATA, NULL
      };
    }

  g_mutex_lock(self->lock);
  if (driver->num_of_messages_to_generate <= 0)
    {
      g_cond_signal(self->cond);
      g_mutex_unlock(self->lock);
      return (LogThreadedFetchResult)
      {
        THREADED_FETCH_ERROR, NULL
      };
    }

  LogMessage *msg = create_sample_message();

  driver->num_of_messages_to_generate--;
  g_mutex_unlock(self->lock);

  return (LogThreadedFetchResult)
  {
    .result = THREADED_FETCH_SUCCESS,
    .msg = msg
  };
}

Test(logthrfetcherdrv, test_no_data)
{
  TestThreadedSourceDriver *s = test_threaded_source_driver_new(main_loop_get_current_config(main_loop));
  s->no_data_first_time = TRUE;

  s->num_of_messages_to_generate = 1;
  parameters.time_reopen = 0;
  parameters.no_data_delay = 1;
  parameters.fetch = _fetch_for_no_data;

  struct timespec start = {0};
  cr_assert(!clock_gettime(CLOCK_MONOTONIC, &start));

  start_test_threaded_source_driver(s);
  wait_for_messages(s);
  stop_test_threaded_source_driver(s);

  struct timespec stop = {0};
  cr_assert(!clock_gettime(CLOCK_MONOTONIC, &stop));

  cr_assert(stop.tv_sec - start.tv_sec >= 1);

  destroy_test_threaded_source_driver(s);
}
