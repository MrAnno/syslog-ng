/*
 * Copyright (c) 2021 One Identity
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

#include "stats/stats-aggregator.h"
#include "stats/stats-registry.h"
#include "stats/stats-cluster-single.h"

typedef struct
{
  StatsAggregator super;
  StatsCounterItem *output_counter;
} StatsAggregatedMaximum;

static void
_unregister_counter(StatsAggregatedMaximum *self)
{
  if(self->output_counter)
    {
      stats_lock();
      stats_unregister_counter(&self->super.key, SC_TYPE_SINGLE_VALUE, &self->output_counter);
      stats_unlock();
    }
}

static void
_regist_counter(StatsAggregatedMaximum *self)
{
  stats_lock();
  stats_register_counter(self->super.stats_level, &self->super.key, SC_TYPE_SINGLE_VALUE, &self->output_counter);
  stats_unlock();
}

static void
_insert_data(StatsAggregator *s, gsize value)
{
  StatsAggregatedMaximum *self = (StatsAggregatedMaximum *)s;
  gsize current_max = 0;

  do
    {
      current_max = stats_counter_get(self->output_counter);

      if (current_max >= value)
        break;

    }
  while(!atomic_gssize_compare_and_exchange(&self->output_counter->value, current_max, value));
}

static void
_registry(StatsAggregator *s)
{
  StatsAggregatedMaximum *self = (StatsAggregatedMaximum *)s;
  _regist_counter(self);
}

static void
_unregistry(StatsAggregator *s)
{
  StatsAggregatedMaximum *self = (StatsAggregatedMaximum *)s;
  _unregister_counter(self);
}

static void
_set_virtual_function(StatsAggregatedMaximum *self)
{
  self->super.insert_data = _insert_data;
  self->super.registry = _registry;
  self->super.unregistry = _unregistry;
}

StatsAggregator *
stats_aggregator_maximum_new(gint level, StatsClusterKey *sc_key)
{
  StatsAggregatedMaximum *self = g_new0(StatsAggregatedMaximum, 1);
  stats_aggregator_init_instance(&self->super, sc_key, level);
  _set_virtual_function(self);

  return &self->super;
}
