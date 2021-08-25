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
  StatsCounterItem *item;
  atomic_gssize count;
  atomic_gssize sum;
} StatsAggregatedAverageItem;

static inline void
_inc_count(StatsAggregatedAverageItem *self)
{
  atomic_gssize_inc(&self->count);
}

static inline void
_add_sum(StatsAggregatedAverageItem *self, gsize value)
{
  atomic_gssize_add(&self->sum, value);
}

static inline gsize
_get_sum(StatsAggregatedAverageItem *self)
{
  return (gsize)atomic_gssize_get(&self->sum);
}

static inline gsize
_get_count(StatsAggregatedAverageItem *self)
{
  return (gsize)atomic_gssize_get(&self->count);
}

static void
_unregist_counter(StatsAggregatedAverageItem *self)
{
  if(self->item)
    {
      stats_lock();
      stats_unregister_counter(&self->super.key, SC_TYPE_SINGLE_VALUE, &self->item);
      stats_unlock();
    }
}

static void
_free(StatsAggregator *s)
{
  StatsAggregatedAverageItem *self = (StatsAggregatedAverageItem *)s;
  _unregist_counter(self);
}

static void
_aggregate(StatsAggregator *s)
{
  StatsAggregatedAverageItem *self = (StatsAggregatedAverageItem *)s;
  stats_counter_set(self->item, (_get_sum(self) / _get_count(self)));
}

static void
_insert_data(StatsAggregator *s, gsize value)
{
  StatsAggregatedAverageItem *self = (StatsAggregatedAverageItem *)s;

  if (self != NULL)
    {
      _inc_count(self);
      _add_sum(self, value);
      _aggregate(s);
    }
}

static void
_reset(StatsAggregator *s)
{
  StatsAggregatedAverageItem *self = (StatsAggregatedAverageItem *)s;
  atomic_gssize_set(&self->sum, 0);
  atomic_gssize_set(&self->count, 0);
}

static void
_set_virtual_function(StatsAggregatedAverageItem *self )
{
  self->super.insert_data = _insert_data;
  self->super.free = _free;
  self->super.reset = _reset;
}

static void
_regist_counter(StatsAggregatedAverageItem *self, gint level, StatsClusterKey *sc_key)
{
  stats_lock();
  stats_register_counter(level, &self->super.key, SC_TYPE_SINGLE_VALUE, &self->item);
  stats_unlock();
}

StatsAggregator *
stats_aggregat_average_new(gint level, StatsClusterKey *sc_key)
{
  level = 0;
  StatsAggregatedAverageItem *self = g_new0(StatsAggregatedAverageItem, 1);
  stats_aggregator_item_init_instance(&self->super, sc_key);
  _set_virtual_function(self);
  _regist_counter(self, level, sc_key);

  return &self->super;
}
