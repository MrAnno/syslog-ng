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

gboolean
stats_aggregator_is_orphaned(StatsAggregator *self)
{
  if (self)
    return self->use_count <= 0;
  return TRUE;
}

void
stats_aggregator_track_counter(StatsAggregator *self)
{
  if (self)
    ++self->use_count;
}

void
stats_aggregator_untrack_counter(StatsAggregator *self)
{
  if (self)
    --self->use_count;
}


void
stats_aggregator_insert_data(StatsAggregator *self, gsize value)
{
  if (self && self->insert_data)
    self->insert_data(self, value);
}

void
stats_aggregator_aggregate(StatsAggregator *self)
{
  if (self && self->aggregate)
    self->aggregate(self);
}

void
stats_aggregator_reset(StatsAggregator *self)
{
  if (self && self->reset)
    self->reset(self);
}

void
stats_aggregator_free(StatsAggregator *self)
{
  if (self && self->free)
    self->free(self);
  stats_cluster_key_cloned_free(&self->key);
  g_free(self);
}

void
stats_aggregator_item_init_instance(StatsAggregator *self, StatsClusterKey *sc_key)
{
  self->use_count = 0;
  stats_cluster_key_clone(&self->key, sc_key);
}