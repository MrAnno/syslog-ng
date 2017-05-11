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
#include "varbindlist-scanner.h"
#include "str-format.h"
#include "timeutils.h"

#include <string.h>
#include <ctype.h>

typedef struct _SnmpTrapdParser
{
  LogParser super;
  GString *prefix;
  LogTemplate *message_template;
  LogTemplateOptions template_options;

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

// DUPLICATED
static inline void
_skip_whitespaces(const gchar **input, gsize *input_len)
{
  const gchar *current_char = *input;

  while (*input_len > 0 && g_ascii_isspace(*current_char))
    {
      ++current_char;
      --(*input_len);
    }

  *input = current_char;
}

/*static
_try_parse_v1_info()
{
  _try_v1_scan_enterprise_string();
  \n
    _try_v1_scan_trap_description_and_subtype();
    _try_v1_scan_uptime();
    \n
}*/

static gboolean
_parse_timestamp(LogMessage *msg, const gchar **input, gsize *input_len)
{
  GTimeVal now;
  cached_g_current_time(&now);
  time_t now_tv_sec = (time_t) now.tv_sec;

  LogStamp *stamp = &msg->timestamps[LM_TS_STAMP];
  stamp->tv_usec = 0;
  stamp->zone_offset = -1;

  /* NOTE: we initialize various unportable fields in tm using a
   * localtime call, as the value of tm_gmtoff does matter but it does
   * not exist on all platforms and 0 initializing it causes trouble on
   * time-zone barriers */

  struct tm tm;
  cached_localtime(&now_tv_sec, &tm);
  if (!scan_std_timestamp(input, (gint *)input_len, &tm))
    return FALSE;

  stamp->tv_sec = cached_mktime(&tm);
  stamp->zone_offset = get_local_timezone_ofs(stamp->tv_sec);

  return TRUE;
}

static gboolean
_parse_header(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  _skip_whitespaces(input, input_len);

  if (!_parse_timestamp(msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  /*
  _parse_hostname();
  _parse_transport_info();
  :
  _try_parse_v1_info();*/



  const gchar *next_line = strchr(*input, '\n');
  if (!next_line)
    return FALSE;

  *input_len -= (next_line + 1) - *input;
  *input = next_line + 1;

  return TRUE;
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

  msg_trace("snmptrapd_parser_process", evt_tag_str("input", input));

  log_msg_make_writable(pmsg, path_options);

  /* APPEND_ZERO(input, input, input_len);? */

  if (!_parse_header(self, *pmsg, &input, &input_len) || !_parse_varbindlist(self, *pmsg, &input, &input_len))
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

  log_template_unref(self->message_template);
  g_string_free(self->prefix, TRUE);
  g_string_free(self->formatted_key, TRUE);

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
  self->super.process = _process_threaded;

  log_template_options_defaults(&self->template_options);

  self->prefix = g_string_new(".snmp.");
  self->formatted_key = g_string_sized_new(32);

  return &self->super;
}
