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
#include "snmptrapd-header-parser.h"
#include "varbindlist-scanner.h"

typedef struct _SnmpTrapdParser
{
  LogParser super;
  GString *prefix;

  GString *formatted_key;
  VarBindListScanner *varbindlist_scanner;
} SnmpTrapdParser;

void
snmptrapd_parser_set_prefix(LogParser *s, const gchar *prefix)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  if (!prefix)
    g_string_truncate(self->prefix, 0);
  else
    g_string_assign(self->prefix, prefix);

  msg_trace("snmptrapd_parser_set_prefix", evt_tag_str("prefix", self->prefix->str));
}

static const gchar *
_get_formatted_key(SnmpTrapdParser *self, const gchar *key)
{
  if (self->prefix->len == 0)
    return key;

  if (self->formatted_key->len > 0)
    g_string_truncate(self->formatted_key, self->prefix->len);
  else
    g_string_assign(self->formatted_key, self->prefix->str);

  g_string_append(self->formatted_key, key);
  return self->formatted_key->str;
}

static gboolean
_parse_varbindlist(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  const gchar *key, *value;

  varbindlist_scanner_input(self->varbindlist_scanner, *input);
  while (varbindlist_scanner_scan_next(self->varbindlist_scanner))
    {
      key = _get_formatted_key(self, varbindlist_scanner_get_current_key(self->varbindlist_scanner));
      value = varbindlist_scanner_get_current_value(self->varbindlist_scanner);

      log_msg_set_value_by_name(msg, key, value, -1);

      msg_debug("----------------", evt_tag_str("key", key), evt_tag_str("value", log_msg_get_value_by_name(msg, key, NULL)));

      msg_trace("varbindlist_scanner_scan_next",
                evt_tag_str("key", key),
                evt_tag_str("type", varbindlist_scanner_get_current_type(self->varbindlist_scanner)),
                evt_tag_str("value", value));
    }

  return TRUE;
}

static gboolean
snmptrapd_parser_process(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                         const gchar *input, gsize input_len)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;
  log_msg_make_writable(pmsg, path_options);

  /* APPEND_ZERO(input, input, input_len);? */

  SnmpTrapdHeaderParser header_parser =
  {
    .key_prefix = self->prefix,
    .msg = *pmsg,
    .input = &input,
    .input_len = &input_len,
    .formatted_key = self->formatted_key
  };

  if (!snmptrapd_parse_header(&header_parser))
    return FALSE;

  if (!_parse_varbindlist(self, *pmsg, &input, &input_len))
    return FALSE;

  /* set default msg */

  return TRUE;
}

/* WORKAROUND, TODO: threadsafe scanner, formatted_key, etc. */
static gboolean
_process_threaded(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                  const gchar *input, gsize input_len)
{
  LogParser *self = (LogParser *)log_pipe_clone(&s->super);

  gboolean ok = snmptrapd_parser_process(self, pmsg, path_options, input, input_len);

  log_pipe_unref(&self->super);
  return ok;
}

static LogPipe *
snmptrapd_parser_clone(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  SnmpTrapdParser *cloned = (SnmpTrapdParser *) snmptrapd_parser_new(s->cfg);

  snmptrapd_parser_set_prefix(&cloned->super, self->prefix->str);

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

  g_string_free(self->prefix, TRUE);
  g_string_free(self->formatted_key, TRUE);

  varbindlist_scanner_free(self->varbindlist_scanner);

  log_parser_free_method(s);
}

static gboolean
snmptrapd_parser_init(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

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
  self->super.process = _process_threaded;

  self->prefix = g_string_new(".snmp.");
  self->formatted_key = g_string_sized_new(32);

  return &self->super;
}
