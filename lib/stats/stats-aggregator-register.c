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

#include "stats/stats-aggregator-register.h"
#include "stats/stats-registry.h"
#include "stats/stats-query.h"
#include "cfg.h"


typedef struct _StatsClusterContainer
{
  GHashTable *clusters;
} StatsAggregatorClusterContainer;

static StatsAggregatorClusterContainer stats_cluster_container;

static GStaticMutex stats_aggregator_mutex = G_STATIC_MUTEX_INIT;
static gboolean stats_aggregator_locked;


void
stats_aggregator_lock(void)
{
  g_static_mutex_lock(&stats_aggregator_mutex);
  stats_aggregator_locked = TRUE;
}

void
stats_aggregator_unlock(void)
{
  stats_aggregator_locked = FALSE;
  g_static_mutex_unlock(&stats_aggregator_mutex);
}

static guint
_stats_cluster_key_hash(const StatsClusterKey *self)
{
  return g_str_hash(self->id) + g_str_hash(self->instance) + self->component;
}

void
stats_aggregator_registry_init(void)
{
  stats_cluster_container.clusters = g_hash_table_new_full((GHashFunc) _stats_cluster_key_hash,
                                                           (GEqualFunc) stats_cluster_key_equal, NULL, NULL);
  _init_timer();
  g_static_mutex_init(&stats_aggregator_mutex);
}

void
stats_aggregator_registry_deinit(void)
{
  g_hash_table_destroy(stats_cluster_container.clusters);
  stats_cluster_container.clusters = NULL;
  g_static_mutex_free(&stats_aggregator_mutex);
}

static void
_insert_to_table(StatsAggregator *value)
{
  g_hash_table_insert(stats_cluster_container.clusters, &value->key, value);
}

static gboolean
_is_in_table(StatsClusterKey *sc_key)
{
  return g_hash_table_lookup(stats_cluster_container.clusters, sc_key) != NULL;
}

static void
_remove_from_table(StatsClusterKey *sc_key)
{
  g_hash_table_remove(stats_cluster_container.clusters, sc_key);
}

static StatsAggregator *
_get_from_table(StatsClusterKey *sc_key)
{
  return g_hash_table_lookup(stats_cluster_container.clusters, sc_key);
}

static void
_unregister(StatsAggregator *s)
{
  if(s == NULL)
    return;

  stats_aggregator_untrack_counter(s);

  if (stats_aggregator_is_orphaned(s) && _is_in_table(&s->key))
    {
      _remove_from_table(&s->key);
      stats_aggregator_free(s);
    }
}

void
stats_register_aggregator_maximum(gint level, StatsClusterKey *sc_key, StatsAggregator **s)
{
  g_assert(stats_aggregator_locked);

  if (!_is_in_table(sc_key))
    {
      *s = stats_aggregat_maximum_new(level, sc_key);
      _insert_to_table(*s);
    }
  else
    {
      *s = _get_from_table(sc_key);
    }

  stats_aggregator_track_counter(*s);
}

void
stats_unregister_aggregator_maximum(StatsAggregator *s)
{
  g_assert(stats_aggregator_locked);
  _unregister(s);
}

void
stats_register_aggregator_average(gint level, StatsClusterKey *sc_key, StatsAggregator **s)
{
  g_assert(stats_aggregator_locked);

  if (!_is_in_table(sc_key))
    {
      *s = stats_aggregat_average_new(level, sc_key);
      _insert_to_table(*s);
    }
  else
    {
      *s = _get_from_table(sc_key);
    }

  stats_aggregator_track_counter(*s);
}

void
stats_unregister_aggregator_average(StatsAggregator *s)
{
  g_assert(stats_aggregator_locked);
  _unregister(s);
}

void
stats_register_aggregator_cps(gint level, StatsClusterKey *sc_key, StatsCounterItem *counter, StatsAggregator **s)
{
  g_assert(stats_aggregator_locked);

  if (!_is_in_table(sc_key))
    {
      *s = stats_aggregat_cps_new(level, sc_key, counter);
      _insert_to_table(*s);
    }
  else
    {
      *s = _get_from_table(sc_key);
    }

  stats_aggregator_track_counter(*s);
}

void
stats_unregister_aggregator_cps(StatsAggregator *s)
{
  g_assert(stats_aggregator_locked);
  _unregister(s);
}

static void
_reset_func (gpointer _key, gpointer _value, gpointer _user_data)
{
  StatsAggregator *self = (StatsAggregator *) _value;
  stats_aggregator_reset(self);
}

void
stats_aggregator_registry_reset(void)
{
  g_hash_table_foreach(stats_cluster_container.clusters, _reset_func, NULL);
}
