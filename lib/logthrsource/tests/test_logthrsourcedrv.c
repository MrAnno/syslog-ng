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

#include "logthrsource/logthrsourcedrv.h"
#include "apphook.h"
#include "mainloop.h"
#include "mainloop-worker.h"
#include "cfg.h"

typedef struct _TestThreadedSourceDriver
{
  LogThreadedSourceDriver super;
} TestThreadedSourceDriver;

MainLoopOptions main_loop_options = {0};
MainLoop *main_loop;
TestThreadedSourceDriver *source_driver;

static const gchar *
_generate_persist_name(const LogPipe *s)
{
  return "test_threaded_source_driver";
}

static const gchar *
_format_stats_instance(LogThreadedSourceDriver *s)
{
  return "test_threaded_source_driver_stats";
}

static void
_run(LogThreadedSourceDriver *s)
{

}

static void _request_exit(LogThreadedSourceDriver *s) {}

static TestThreadedSourceDriver *
test_threaded_sd_new(GlobalConfig *cfg)
{
  TestThreadedSourceDriver *self = g_new0(TestThreadedSourceDriver, 1);

  log_threaded_source_driver_init_instance(&self->super, cfg);

  log_threaded_source_driver_set_worker_run(&self->super, _run);
  log_threaded_source_driver_set_worker_request_exit(&self->super, _request_exit);

  self->super.format_stats_instance = _format_stats_instance;
  self->super.super.super.super.generate_persist_name = _generate_persist_name;

  return self;
}

static void
setup_threaded_source(void)
{
  source_driver = test_threaded_sd_new(main_loop_get_current_config(main_loop));

  cr_assert(log_pipe_init(&source_driver->super.super.super.super));
}

static void
teardown_threaded_source(void)
{
  main_loop_sync_worker_startup_and_teardown();
  cr_assert(log_pipe_deinit(&source_driver->super.super.super.super));
  log_pipe_unref(&source_driver->super.super.super.super);
}

static void
setup(void)
{
  app_startup();

  main_loop = main_loop_get_instance();
  main_loop_init(main_loop, &main_loop_options);
  setup_threaded_source();
}

static void
teardown(void)
{
  teardown_threaded_source();
  main_loop_deinit(main_loop);
  app_shutdown();
}

TestSuite(logthrsourcedrv, .init = setup, .fini = teardown);

Test(logthrsourcedrv, test_threaded_source_init)
{
  cr_assert(1);
}

