#include "mranno.h"
#include "mranno-source.h"

struct MrAnnoSourceDriver
{
  LogSrcDriver super;
  MrAnnoSource *source;
  MrAnnoSourceOptions source_options;
};

static gboolean
mranno_sd_init(LogPipe *s)
{
  MrAnnoSourceDriver *self = (MrAnnoSourceDriver *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  mranno_source_set_options(self->source, &self->source_options, self->super.super.id, NULL, FALSE, FALSE,
                            self->super.super.super.expr_node);
  log_pipe_append((LogPipe *)self->source, &self->super.super.super);
  mranno_source_options_init(&self->source_options, cfg, self->super.super.group);
  if(!mranno_source_init(self->source))
    return FALSE;

  return log_src_driver_init_method(s);
}

static gboolean
mranno_sd_deinit(LogPipe *s)
{
  MrAnnoSourceDriver *self = (MrAnnoSourceDriver *) s;

  if (!mranno_source_deinit(self->source))
    return FALSE;

  return log_src_driver_deinit_method(s);
}

static void
mranno_sd_free(LogPipe *s)
{
  MrAnnoSourceDriver *self = (MrAnnoSourceDriver *) s;

  mranno_source_free(self->source);
  mranno_source_options_destroy(&self->source_options);
  log_src_driver_free(s);
}

MrAnnoSourceOptions *
mranno_sd_get_source_options(LogDriver *s)
{
  MrAnnoSourceDriver *self = (MrAnnoSourceDriver *) s;

  return &self->source_options;
}

LogDriver *
mranno_sd_new(GlobalConfig *cfg)
{
  MrAnnoSourceDriver *self = g_new0(MrAnnoSourceDriver, 1);

  log_src_driver_init_instance(&self->super, cfg);
  mranno_source_options_defaults(&self->source_options);

  self->source = mranno_source_new(cfg);

  self->super.super.super.init = mranno_sd_init;
  self->super.super.super.deinit = mranno_sd_deinit;
  self->super.super.super.free_fn = mranno_sd_free;

  return &self->super.super;
}
