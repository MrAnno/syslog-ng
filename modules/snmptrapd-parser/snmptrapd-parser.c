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
#include "varbindlist-scanner.h"

#include <string.h>
#include <ctype.h>

typedef struct _SnmpTrapdParser
{
  LogParser super;
  gchar *prefix;
  LogTemplate *message_template;
  LogTemplateOptions template_options;

  VarBindListScanner *varbindlist_scanner;
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
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  msg_trace("snmptrapd_parser_process", evt_tag_str("input", input));

  log_msg_make_writable(pmsg, path_options);

  /* header scanner */
  /* ... */

  /* input_len? nullterminálás garantálva? */
  const gchar *next_line = strchr(input, '\n');
  if (!next_line)
    return FALSE;

  varbindlist_scanner_input(self->varbindlist_scanner, input);
  while (varbindlist_scanner_scan_next(self->varbindlist_scanner))
    {
      msg_trace("varbindlist_scanner_scan_next",
                evt_tag_str("key", varbindlist_scanner_get_current_key(self->varbindlist_scanner)),
                evt_tag_str("type", varbindlist_scanner_get_current_type(self->varbindlist_scanner)),
                evt_tag_str("value", varbindlist_scanner_get_current_value(self->varbindlist_scanner)));
    }

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

  if (self->varbindlist_scanner)
    cloned->varbindlist_scanner = varbindlist_scanner_clone(self->varbindlist_scanner);

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

  varbindlist_scanner_free(self->varbindlist_scanner);

  log_parser_free_method(s);
}

static gboolean
snmptrapd_parser_init(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  log_template_options_init(&self->template_options, cfg);

  g_assert(self->varbindlist_scanner == NULL);
  self->varbindlist_scanner = varbindlist_scanner_new();

  return log_parser_init_method(s);
}

static gboolean
snmptrapd_parser_deinit(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *)s;

  varbindlist_scanner_free(self->varbindlist_scanner);
  self->varbindlist_scanner = NULL;
  return TRUE;
}

LogParser *
snmptrapd_parser_new(GlobalConfig *cfg)
{
  SnmpTrapdParser *self = g_new0(SnmpTrapdParser, 1);

  log_parser_init_instance(&self->super, cfg);
  self->super.super.init = snmptrapd_parser_init;
  self->super.super.deinit = snmptrapd_parser_deinit;
  self->super.super.free_fn = snmptrapd_parser_free;
  self->super.super.clone = snmptrapd_parser_clone;
  self->super.process = snmptrapd_parser_process;

  log_template_options_defaults(&self->template_options);

  return &self->super;
}
