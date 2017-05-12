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
#include "str-format.h"
#include "timeutils.h"

#include <string.h>
#include <ctype.h>

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

// DUPLICATED
static inline void
_skip_whitespaces(const gchar **input, gsize *input_len)
{
  const gchar *current_char = *input;

  while (*input_len > 0 && (*current_char == ' ' || *current_char == '\t'))
    {
      ++current_char;
      --(*input_len);
    }

  *input = current_char;
}

static gboolean
_parse_v1_enterprise_oid(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  const gchar *enterprise_string_start = *input;
  gsize input_left = *input_len;

  while (*input_len > 0 && !g_ascii_isspace(**input))
    {
      ++(*input);
      --(*input_len);
    }

  gsize enterprise_string_length = input_left - *input_len;
  // try
  if (enterprise_string_length == 0)
    return TRUE;

  if (**input != '\n')
    _skip_whitespaces(input, input_len);

  if (**input != '\n')
    return FALSE;

  log_msg_set_value_by_name(msg, _get_formatted_key(self, "enterprise_oid"),
                            enterprise_string_start, enterprise_string_length);

  return TRUE;
}

static gboolean
_parse_v1_trap_type_and_subtype(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  const gchar *type_start = *input;

  const gchar *type_end = strpbrk(type_start, "(\n");
  gboolean type_exists = type_end && *type_end == '(';

  if (!type_exists)
    return FALSE;

  const gchar *subtype_start = type_end + 1;

  if (*(type_end - 1) == ' ')
    --type_end;

  log_msg_set_value_by_name(msg, _get_formatted_key(self, "type"),
                            type_start, type_end - type_start);

  const gchar *subtype_end = strpbrk(subtype_start, ")\n");
  gboolean subtype_exists = subtype_end && *subtype_end == ')';

  if (!subtype_exists)
    return FALSE;

  log_msg_set_value_by_name(msg, _get_formatted_key(self, "subtype"),
                            subtype_start, subtype_end - subtype_start);


  subtype_end++;

  *input_len -= subtype_end - *input;
  *input = subtype_end;

  return TRUE;
}

static gboolean
_parse_v1_uptime(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  if (!scan_expect_str(input, (gint *)input_len, "Uptime:"))
    return FALSE;

  _skip_whitespaces(input, input_len);

  const gchar *uptime_start = *input;

  const gchar *uptime_end = strchr(uptime_start, '\n');
  if (!uptime_end)
    return FALSE;

  log_msg_set_value_by_name(msg, _get_formatted_key(self, "uptime"),
                            uptime_start, uptime_end - uptime_start);

  *input_len -= uptime_end - uptime_start;
  *input = uptime_end;

  return TRUE;
}

static gboolean
_try_parse_v1_info(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  if (!_parse_v1_enterprise_oid(self, msg, input, input_len))
    return FALSE;

  //ez megeszi a v2 header újsorát is
  if(!scan_expect_char(input, (gint *)input_len, '\n'))
    return FALSE;

  //try
  if(!scan_expect_char(input, (gint *)input_len, '\t'))
    return TRUE;

  _skip_whitespaces(input, input_len);

  if (!_parse_v1_trap_type_and_subtype(self, msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if (!_parse_v1_uptime(self, msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if(!scan_expect_char(input, (gint *)input_len, '\n'))
    return FALSE;

  return TRUE;
}

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
_parse_hostname(LogMessage *msg, const gchar **input, gsize *input_len)
{
  const gchar *hostname_start = *input;
  gsize input_left = *input_len;

  while (*input_len > 0 && !g_ascii_isspace(**input))
    {
      ++(*input);
      --(*input_len);
    }

  gsize hostname_length = input_left - *input_len;
  if (hostname_length == 0)
    return FALSE;

  log_msg_set_value(msg, LM_V_HOST, hostname_start, hostname_length);
  return TRUE;
}

static gboolean
_parse_transport_info(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  if(!scan_expect_char(input, (gint *) input_len, '['))
    return FALSE;

  _skip_whitespaces(input, input_len);

  const gchar *start_pos = *input;
  gchar *curr_char = strchr(start_pos, '\n');
  while(*curr_char != ']')
    {
      --curr_char;
      if(curr_char == start_pos)
        return FALSE;
    }
  gsize transport_info_len = curr_char - start_pos;

  *input += transport_info_len + 1;
  log_msg_set_value_by_name(msg, _get_formatted_key(self, "transport_info"), start_pos, transport_info_len);
  return TRUE;
}

static gboolean
_parse_header(SnmpTrapdParser *self, LogMessage *msg, const gchar **input, gsize *input_len)
{
  _skip_whitespaces(input, input_len);

  if (!_parse_timestamp(msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if (!_parse_hostname(msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if(!_parse_transport_info(self, msg, input, input_len))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if(!scan_expect_char(input, (gint *) input_len, ':'))
    return FALSE;

  _skip_whitespaces(input, input_len);

  if (!_try_parse_v1_info(self, msg, input, input_len))
    return FALSE;

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
