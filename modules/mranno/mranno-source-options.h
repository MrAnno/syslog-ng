#ifndef MRANNO_SOURCE_OPTIONS_H
#define MRANNO_SOURCE_OPTIONS_H

#include "logsource.h"

typedef struct
{
  LogSourceOptions super;
  guint message_interval;
} MrAnnoSourceOptions;

static inline void
mranno_source_options_defaults(MrAnnoSourceOptions *self)
{
  log_source_options_defaults(&self->super);
  self->message_interval = 5;
}

static inline void
mranno_source_options_init(MrAnnoSourceOptions *self, GlobalConfig *cfg, const gchar *group_name)
{
  log_source_options_init(&self->super, cfg, group_name);
}

static inline void
mranno_source_options_destroy(MrAnnoSourceOptions *self)
{
  log_source_options_destroy(&self->super);
}

static inline void
mranno_source_options_set_message_interval(MrAnnoSourceOptions *self, guint message_interval)
{
  self->message_interval = message_interval;
}

#endif /* MRANNO_SOURCE_OPTIONS_H */
