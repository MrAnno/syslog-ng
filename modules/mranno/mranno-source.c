#include "mranno-source.h"

#include "logsource.h"
#include "logpipe.h"
#include "logmsg/logmsg.h"

#include <iv.h>

struct MrAnnoSource
{
  LogSource super;
  MrAnnoSourceOptions *options;
  struct iv_timer timer;
};

static void
_stop_timer(MrAnnoSource *self)
{
  if (iv_timer_registered(&self->timer))
    iv_timer_unregister(&self->timer);
}

static void
_start_timer(MrAnnoSource *self)
{
  _stop_timer(self);

  iv_validate_now();
  self->timer.expires = iv_now;
  self->timer.expires.tv_sec += self->options->message_interval;

  iv_timer_register(&self->timer);
}

static gboolean
_init(LogPipe *s)
{
  MrAnnoSource *self = (MrAnnoSource *) s;
  _start_timer(self);
  return log_source_init(s);
}

static gboolean
_deinit(LogPipe *s)
{
  MrAnnoSource *self = (MrAnnoSource *) s;

  _stop_timer(self);

  return log_source_deinit(s);
}

static void
_free(LogPipe *s)
{
  log_source_free(s);
}

static void
_send_generated_message(MrAnnoSource *self)
{
  if (log_source_free_to_send(&self->super))
    {
      LogMessage *msg = log_msg_new_internal(LOG_SYSLOG | LOG_INFO, "Hello, it's me.");
      log_source_post(&self->super, msg);
    }
}

static void
_timer_expired(void *cookie)
{
  MrAnnoSource *self = (MrAnnoSource *) cookie;

  _send_generated_message(self);

  _start_timer(self);
}

gboolean
mranno_source_init(MrAnnoSource *self)
{
  return log_pipe_init(&self->super.super);
}

gboolean
mranno_source_deinit(MrAnnoSource *self)
{
  return log_pipe_deinit(&self->super.super);
}

void
mranno_source_free(MrAnnoSource *self)
{
  log_pipe_unref(&self->super.super);
}

void
mranno_source_set_options(MrAnnoSource *self, MrAnnoSourceOptions *options,
                          const gchar *stats_id, const gchar *stats_instance,
                          gboolean threaded, gboolean pos_tracked, LogExprNode *expr_node)
{
  self->options = options;
  log_source_set_options(&self->super, &options->super, stats_id, stats_instance, threaded, pos_tracked, expr_node);
}

MrAnnoSource *
mranno_source_new(GlobalConfig *cfg)
{
  MrAnnoSource *self = g_new0(MrAnnoSource, 1);
  log_source_init_instance(&self->super, cfg);

  IV_TIMER_INIT(&self->timer);
  self->timer.cookie = self;
  self->timer.handler = _timer_expired;

  self->super.super.init = _init;
  self->super.super.deinit = _deinit;
  self->super.super.free_fn = _free;

  return self;
}
