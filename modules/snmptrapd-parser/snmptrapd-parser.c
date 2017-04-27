/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 Gabor Nagy <gabor.nagy@balabit.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 */
#include "snmptrapd-parser.h"

#include <string.h>
#include <ctype.h>

typedef struct _SnmpTrapdParser
{
  LogParser super;
  gchar *prefix;
  LogTemplate *message_template;
  LogTemplateOptions template_options;
} SnmpTrapdParser;

void
snmptrapd_parser_set_prefix(LogParser *s, const gchar *prefix)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  g_free(self->prefix);
  self->prefix = g_strdup(prefix);

  msg_trace("snmptrapd_parser_set_prefix", evt_tag_str("prefix", self->prefix));
}

void
snmptrapd_parser_set_message_template(LogParser *s, LogTemplate *message_template)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  log_template_unref(self->message_template);
  self->message_template = log_template_ref(message_template);

  msg_trace("snmptrapd_parser_set_message_template", evt_tag_str("message_template", self->message_template->template));
}

LogTemplateOptions *
snmptrapd_parser_get_template_options(LogParser *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  return &self->template_options;
}

static gboolean
snmptrapd_parser_process(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                         const gchar *input, gsize input_len)
{
  return TRUE;
}

static LogPipe *
snmptrapd_parser_clone(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  SnmpTrapdParser *cloned = (SnmpTrapdParser *) snmptrapd_parser_new(s->cfg);

  snmptrapd_parser_set_prefix(&cloned->super, self->prefix);
  snmptrapd_parser_set_message_template(&cloned->super, self->message_template);

  /* log_parser_clone_method() is missing.. */
  log_parser_set_template(&cloned->super, log_template_ref(self->super.template));

  msg_trace("snmptrapd_parser_clone");

  return &cloned->super.super;
}

static void
snmptrapd_parser_free(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  log_template_options_destroy(&self->template_options);

  g_free(self->prefix);
  log_template_unref(self->message_template);

  log_parser_free_method(s);
}

static gboolean
snmptrapd_parser_init(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  log_template_options_init(&self->template_options, cfg);

  return log_parser_init_method(s);
}


LogParser *
snmptrapd_parser_new(GlobalConfig *cfg)
{
  SnmpTrapdParser *self = g_new0(SnmpTrapdParser, 1);

  log_parser_init_instance(&self->super, cfg);
  self->super.super.init = snmptrapd_parser_init;
  self->super.super.free_fn = snmptrapd_parser_free;
  self->super.super.clone = snmptrapd_parser_clone;
  self->super.process = snmptrapd_parser_process;

  log_template_options_defaults(&self->template_options);

  return &self->super;
}
