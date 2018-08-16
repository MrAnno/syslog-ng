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

#include "logthrsource/logthrfetcherdrv.h"
#include "apphook.h"
#include "mainloop.h"
#include "mainloop-worker.h"
#include "cfg.h"

typedef struct _TestThreadedFetcherDriver
{
  LogThreadedFetcherDriver super;
} TestThreadedFetcherDriver;

MainLoopOptions main_loop_options = {0};
MainLoop *main_loop;
TestThreadedFetcherDriver *source_driver;

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

static LogThreadedFetchResult
_fetch(LogThreadedFetcherDriver *s)
{
  return (LogThreadedFetchResult) {
    .result = THREADED_FETCH_SUCCESS,
    .msg = log_msg_new_empty()
  };
}

static TestThreadedFetcherDriver *
test_threaded_fetcher_new(GlobalConfig *cfg)
{
  TestThreadedFetcherDriver *self = g_new0(TestThreadedFetcherDriver, 1);

  log_threaded_fetcher_driver_init_instance(&self->super, cfg);

  self->super.fetch = _fetch;

  self->super.super.format_stats_instance = _format_stats_instance;
  self->super.super.super.super.super.generate_persist_name = _generate_persist_name;

  return self;
}

static void
setup_threaded_fetcher(void)
{
  source_driver = test_threaded_fetcher_new(main_loop_get_current_config(main_loop));

  cr_assert(log_pipe_init(&source_driver->super.super.super.super.super));
}

static void
teardown_threaded_fetcher(void)
{
  main_loop_sync_worker_startup_and_teardown();
  cr_assert(log_pipe_deinit(&source_driver->super.super.super.super.super));
  log_pipe_unref(&source_driver->super.super.super.super.super);
}

static void
setup(void)
{
  app_startup();

  main_loop = main_loop_get_instance();
  main_loop_init(main_loop, &main_loop_options);
  setup_threaded_fetcher();
}

static void
teardown(void)
{
  teardown_threaded_fetcher();
  main_loop_deinit(main_loop);
  app_shutdown();
}

TestSuite(logthrfetcherdrv, .init = setup, .fini = teardown);

Test(logthrfetcherdrv, test_threaded_fetcher_init)
{
  cr_assert(1);
}

