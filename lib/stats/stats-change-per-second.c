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
#include "timeutils/cache.h"
#include "stats/stats-registry.h"
#include "stats/stats-cluster-single.h"
#include "timeutils/cache.h"
#include <math.h>

#define HOUR_IN_SEC 3600 /* 60*60 */
#define DAY_IN_SEC 86400 /* 60*60*24 */

typedef struct
{
  StatsCounterItem *output_counter;
  atomic_gssize average;
  atomic_gssize sum;
  atomic_gssize last_count;

  gssize duration; /* if the duration equals -1, thats mean, it count since syslog start */
  gchar *name;
} CPSLogic;
/* CPS == Change Per Second */

typedef struct
{
  StatsAggregator super;
  time_t init_time;
  time_t last_add_time;

  StatsCounterItem *input_counter;

  CPSLogic hour;
  CPSLogic day;
  CPSLogic start;
} StatsAggregatorCPS;

static inline gsize
_get_count(StatsAggregatorCPS *self)
{
  return stats_counter_get(self->input_counter);
}

static inline void
_set_sum(CPSLogic *self, gsize set)
{
  atomic_gssize_set(&self->sum, set);
}

static inline void
_add_to_sum(CPSLogic *self, gsize value)
{
  atomic_gssize_add(&self->sum, value);
}

static inline gsize
_get_sum(CPSLogic *self)
{
  return atomic_gssize_get_unsigned(&self->sum);
}

static inline void
_set_average(CPSLogic *self, gsize set)
{
  atomic_gssize_set(&self->average, set);
}

static inline gsize
_get_average(CPSLogic *self)
{
  return atomic_gssize_get_unsigned(&self->average);
}

static inline void
_set_last_count(CPSLogic *self, gsize set)
{
  atomic_gssize_set(&self->last_count, set);
}

static inline gsize
_get_last_count(CPSLogic *self)
{
  return atomic_gssize_get_unsigned(&self->last_count);
}

static inline gdouble
_calc_sec_between_time(time_t *start, time_t *end)
{
  return (difftime(*end, *start));
}

static gboolean
_is_less_then_duration(StatsAggregatorCPS *self, CPSLogic *logic, time_t *now)
{
  if (logic->duration == -1)
    return TRUE;

  return _calc_sec_between_time(&self->init_time, now) <= logic->duration;
}

static void
_calc_sum(StatsAggregatorCPS *self, CPSLogic *logic, time_t *now)
{
  gsize diff = _get_count(self) - _get_last_count(logic);
  _set_last_count(logic, _get_count(self));

  if (!_is_less_then_duration(self, logic, now))
    {
      gsize elapsed_time_since_last = (gsize)_calc_sec_between_time(&self->last_add_time, now);
      diff -= _get_average(logic) * elapsed_time_since_last;
    }

  _add_to_sum(logic, diff);
  self->last_add_time = *now;
}

static void
_calc_average(StatsAggregatorCPS *self, CPSLogic *logic, time_t *now)
{
  gsize elipsed_time = (gsize)_calc_sec_between_time(&self->init_time, now);
  gsize to_divede = (_is_less_then_duration(self, logic, now)) ? elipsed_time : logic->duration;
  if (to_divede <= 0) to_divede = 1;

  _set_average(logic, (_get_sum(logic) / to_divede));
}

static void
_aggregate_CPS_logic(StatsAggregatorCPS *self, CPSLogic *logic, time_t *now)
{
  _calc_sum(self, logic, now);
  _calc_average(self, logic, now);
  stats_counter_set(logic->output_counter, _get_average(logic));
}

static void
_aggregate(StatsAggregator *s)
{
  StatsAggregatorCPS *self = (StatsAggregatorCPS *)s;

  time_t now = cached_g_current_time_sec();
  _aggregate_CPS_logic(self, &self->hour, &now);
  _aggregate_CPS_logic(self, &self->day, &now);
  _aggregate_CPS_logic(self, &self->start, &now);
}

static void
_deinit_CPS_logic(CPSLogic *self, StatsClusterKey *sc_key, gint type)
{
  stats_unregister_counter(sc_key, type, &self->output_counter);
}

static void
_deinit_CPS_logics(StatsAggregatorCPS *self)
{
  stats_lock();
  StatsClusterKey sc_key;

  stats_cluster_single_key_set_with_name(&sc_key, self->super.key.component, self->super.key.id, self->super.key.instance,
                                         self->hour.name);
  _deinit_CPS_logic(&self->hour, &sc_key, SC_TYPE_SINGLE_VALUE);
  g_free(self->hour.name);

  stats_cluster_single_key_set_with_name(&sc_key, self->super.key.component, self->super.key.id, self->super.key.instance,
                                         self->day.name);
  _deinit_CPS_logic(&self->day, &sc_key, SC_TYPE_SINGLE_VALUE);
  g_free(self->day.name);

  stats_cluster_single_key_set_with_name(&sc_key, self->super.key.component, self->super.key.id, self->super.key.instance,
                                         self->start.name);
  _deinit_CPS_logic(&self->start, &sc_key, SC_TYPE_SINGLE_VALUE);
  g_free(self->start.name);

  stats_unlock();
}

static void
_free(StatsAggregator *s)
{
  StatsAggregatorCPS *self = (StatsAggregatorCPS *)s;
  _deinit_CPS_logics(self);
}

static void
_set_values(StatsAggregatorCPS *self, StatsCounterItem *input_counter)
{
  self->init_time = cached_g_current_time_sec();
  self->last_add_time = 0;
  self->input_counter = input_counter;
}

static void
_set_CPS_logic_values(CPSLogic *self, gssize duration)
{
  _set_average(self, 0);
  _set_sum(self, 0);
  _set_last_count(self, 0);
  self->duration = duration;
}

static void
_reset(StatsAggregator *s)
{
  StatsAggregatorCPS *self = (StatsAggregatorCPS *)s;
  _set_values(self, self->input_counter);
  _set_CPS_logic_values(&self->hour, self->hour.duration);
  _set_CPS_logic_values(&self->day, self->day.duration);
  _set_CPS_logic_values(&self->start, self->start.duration);
}

static void
_set_virtual_function(StatsAggregatorCPS *self )
{
  self->super.aggregate = _aggregate;
  self->super.free = _free;
  self->super.reset = _reset;
}


static void
_init_CPS_logic(CPSLogic *self, StatsClusterKey *sc_key, gint level, gint type, gssize duration)
{
  _set_CPS_logic_values(self, duration);
  if(!self->output_counter)
    stats_register_counter(level, sc_key, type, &self->output_counter);
}

static void
_init_CPS_logics(StatsAggregatorCPS *self, gint level, StatsClusterKey *sc_key_orig)
{
  stats_lock();
  StatsClusterKey sc_key;

  self->hour.name = g_strconcat(sc_key_orig->counter_group_init.counter.name, "_since_last_hour", NULL);
  stats_cluster_single_key_set_with_name(&sc_key, sc_key_orig->component, sc_key_orig->id, sc_key_orig->instance,
                                         self->hour.name);
  _init_CPS_logic(&self->hour, &sc_key, level, SC_TYPE_SINGLE_VALUE, HOUR_IN_SEC);

  self->day.name = g_strconcat(sc_key_orig->counter_group_init.counter.name, "_since_last_day", NULL);
  stats_cluster_single_key_set_with_name(&sc_key, sc_key_orig->component, sc_key_orig->id, sc_key_orig->instance,
                                         self->day.name);
  _init_CPS_logic(&self->day, &sc_key, level, SC_TYPE_SINGLE_VALUE, DAY_IN_SEC);

  self->start.name = g_strconcat(sc_key_orig->counter_group_init.counter.name, "_since_begin", NULL);
  stats_cluster_single_key_set_with_name(&sc_key, sc_key_orig->component, sc_key_orig->id, sc_key_orig->instance,
                                         self->start.name);
  _init_CPS_logic(&self->start, &sc_key, level, SC_TYPE_SINGLE_VALUE, -1);

  stats_unlock();
}

StatsAggregator *
stats_aggregator_cps_new(gint level, StatsClusterKey *sc_key, StatsCounterItem *counter)
{
  StatsAggregatorCPS *self = g_new0(StatsAggregatorCPS, 1);
  stats_aggregator_init_instance(&self->super, sc_key);
  _set_virtual_function(self);
  _set_values(self, counter);
  _init_CPS_logics(self, level, sc_key);

  return &self->super;
}

