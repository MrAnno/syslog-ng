/*
 * Copyright (c) 2015 Balabit
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

#include "stats/stats-counter.h"
#include "stats/stats-cluster.h"
#include "stats/stats-registry.h"

static inline void
_reset_counter_if_needed(StatsCluster *sc, gint type, StatsCounter *counter, gpointer user_data)
{
  switch (type)
    {
    case SC_TYPE_QUEUED:
    case SC_TYPE_MEMORY_USAGE:
      return;
    default:
      stats_counter_reset(counter);
    }
}

void
stats_reset_counters(void)
{
  stats_lock();
  stats_foreach_counter(_reset_counter_if_needed, NULL);
  stats_unlock();
}


static void
stats_counter_item_reset(StatsCounter *ctr)
{
  StatsCounterItem *counter = (StatsCounterItem *) ctr;
  stats_counter_set(counter, 0);
}

void
stats_counter_item_init_instance(StatsCounterItem *self)
{
  self->super.get = stats_counter_get;
  self->super.reset = stats_counter_item_reset;
}

void
stats_counter_free(StatsCounter *counter)
{
  g_free(counter->name);
}
