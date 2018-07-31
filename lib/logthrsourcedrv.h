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

#ifndef LOGTHRSOURCEDRV_H
#define LOGTHRSOURCEDRV_H

#include "syslog-ng.h"
#include "driver.h"
#include "logsource.h"
#include "cfg.h"
#include "logpipe.h"

typedef struct _LogThreadedSourceWorker
{
  LogSource super;
} LogThreadedSourceWorker;

typedef struct _LogThreadedSourceDriver
{
  LogSrcDriver super;
  LogThreadedSourceWorker *worker;
} LogThreadedSourceDriver;

void log_threaded_source_driver_init_instance(LogThreadedSourceDriver *self, GlobalConfig *cfg);
gboolean log_threaded_source_driver_init_method(LogPipe *s);
gboolean log_threaded_source_driver_deinit_method(LogPipe *s);
void log_threaded_source_driver_free_method(LogPipe *s);

#endif
