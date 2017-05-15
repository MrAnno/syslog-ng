/*
 * Copyright (c) 2017 Balabit
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
} SnmpTrapdParser;

void
snmptrapd_parser_set_prefix(LogParser *s, const gchar *prefix)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  if (!prefix)
    g_string_truncate(self->prefix, 0);
  else
    g_string_assign(self->prefix, prefix);
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
_parse_header(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  SnmpTrapdHeaderParser header_parser =
  {
    .key_prefix = self->prefix,
    .msg = msg,
    .input = input,
    .input_len = input_len,
    .formatted_key = self->formatted_key
  };

  return snmptrapd_header_parser_parse(&header_parser);
}

static gboolean
_parse_varbindlist(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  VarBindListScanner varbindlist_scanner;
  const gchar *key, *value;

  varbindlist_scanner_init(&varbindlist_scanner);

  varbindlist_scanner_input(&varbindlist_scanner, *input);
  while (varbindlist_scanner_scan_next(&varbindlist_scanner))
    {
      key = _get_formatted_key(self, varbindlist_scanner_get_current_key(&varbindlist_scanner));
      value = varbindlist_scanner_get_current_value(&varbindlist_scanner);

      log_msg_set_value_by_name(msg, key, value, -1);
    }

  varbindlist_scanner_deinit(&varbindlist_scanner);
  return TRUE;
}

static gboolean
snmptrapd_parser_process(LogParser *s, LogMessage **pmsg, const LogPathOptions *path_options,
                         const gchar *input, gsize input_len)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  log_msg_make_writable(pmsg, path_options);

  /* APPEND_ZERO(input, input, input_len);? */

  const gchar *remaining_input = input;
  gsize remaining_input_len = input_len;

  if (!_parse_header(self, *pmsg, &remaining_input, &remaining_input_len))
    return FALSE;

  if (!_parse_varbindlist(self, *pmsg, &remaining_input, &remaining_input_len))
    return FALSE;

  /* set default msg */

  return TRUE;
}

static LogPipe *
snmptrapd_parser_clone(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  SnmpTrapdParser *cloned = (SnmpTrapdParser *) snmptrapd_parser_new(s->cfg);

  snmptrapd_parser_set_prefix(&cloned->super, self->prefix->str);

  /* log_parser_clone_method() is missing.. */
  log_parser_set_template(&cloned->super, log_template_ref(self->super.template));

  return &cloned->super.super;
}

static void
snmptrapd_parser_free(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  g_string_free(self->prefix, TRUE);
  g_string_free(self->formatted_key, TRUE);

  log_parser_free_method(s);
}

LogParser *
snmptrapd_parser_new(GlobalConfig *cfg)
{
  SnmpTrapdParser *self = g_new0(SnmpTrapdParser, 1);

  log_parser_init_instance(&self->super, cfg);
  self->super.super.free_fn = snmptrapd_parser_free;
  self->super.super.clone = snmptrapd_parser_clone;
  self->super.process = snmptrapd_parser_process;

  self->prefix = g_string_new(".snmp.");
  self->formatted_key = g_string_sized_new(32);

  return &self->super;
}
