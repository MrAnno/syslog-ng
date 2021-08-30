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
  atomic_gssize count;
  atomic_gssize sum;
} StatsAggregatedAverage;

static inline void
_inc_count(StatsAggregatedAverage *self)
{
  atomic_gssize_inc(&self->count);
}

static inline void
_add_sum(StatsAggregatedAverage *self, gsize value)
{
  atomic_gssize_add(&self->sum, value);
}

static inline gsize
_get_sum(StatsAggregatedAverage *self)
{
  return atomic_gssize_get_unsigned(&self->sum);
}

static inline gsize
_get_count(StatsAggregatedAverage *self)
{
  return atomic_gssize_get_unsigned(&self->count);
}

static void
_unregister_counter(StatsAggregatedAverage *self)
{
  if(self->output_counter)
    {
      stats_lock();
      stats_unregister_counter(&self->super.key, SC_TYPE_SINGLE_VALUE, &self->output_counter);
      stats_unlock();
    }
}

static void
_regist_counter(StatsAggregatedAverage *self)
{
  stats_lock();
  stats_register_counter(self->super.stats_level, &self->super.key, SC_TYPE_SINGLE_VALUE, &self->output_counter);
  stats_unlock();
}

static void
_insert_data(StatsAggregator *s, gsize value)
{
  StatsAggregatedAverage *self = (StatsAggregatedAverage *)s;

  _inc_count(self);
  _add_sum(self, value);
  stats_counter_set(self->output_counter, (_get_sum(self)/_get_count(self)));
}

static void
_reset(StatsAggregator *s)
{
  StatsAggregatedAverage *self = (StatsAggregatedAverage *)s;
  atomic_gssize_set(&self->count, 0);
}

static void
_registry(StatsAggregator *s)
{
  StatsAggregatedAverage *self = (StatsAggregatedAverage *)s;
  _regist_counter(self);
}

static void
_unregistry(StatsAggregator *s)
{
  StatsAggregatedAverage *self = (StatsAggregatedAverage *)s;
  _unregister_counter(self);
}

static void
_set_virtual_function(StatsAggregatedAverage *self )
{
  self->super.insert_data = _insert_data;
  self->super.reset = _reset;
  self->super.registry = _registry;
  self->super.unregistry = _unregistry;
}


StatsAggregator *
stats_aggregator_average_new(gint level, StatsClusterKey *sc_key)
{
  StatsAggregatedAverage *self = g_new0(StatsAggregatedAverage, 1);
  stats_aggregator_init_instance(&self->super, sc_key, level);
  _set_virtual_function(self);

  return &self->super;
}
