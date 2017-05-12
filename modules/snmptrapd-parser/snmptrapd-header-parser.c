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

#include "snmptrapd-header-parser.h"

#include "str-format.h"
#include "timeutils.h"

#include <string.h>
#include <ctype.h>

typedef gboolean (*SnmpTrapdHeaderParserStep)(SnmpTrapdHeaderParser *header_parser);

static const gchar *
_get_formatted_key(SnmpTrapdHeaderParser *header_parser, const gchar *key)
{
  if (header_parser->key_prefix->len == 0)
    return key;

  if (header_parser->formatted_key->len > 0)
    g_string_truncate(header_parser->formatted_key, header_parser->key_prefix->len);
  else
    g_string_assign(header_parser->formatted_key, header_parser->key_prefix->str);

  g_string_append(header_parser->formatted_key, key);
  return header_parser->formatted_key->str;
}

static inline void
_skip_whitespaces(SnmpTrapdHeaderParser *header_parser)
{
  const gchar *current_char = *header_parser->input;

  while (*header_parser->input_len > 0 && *current_char == ' ')
    {
      ++current_char;
      --(*header_parser->input_len);
    }

  *header_parser->input = current_char;
}


static gboolean
_run_header_parser(SnmpTrapdHeaderParser *header_parser,
                   SnmpTrapdHeaderParserStep *parser_steps, gsize parser_steps_size)
{
  SnmpTrapdHeaderParserStep parser_step;

  for (gsize step_index = 0; step_index < parser_steps_size; ++step_index)
    {
      _skip_whitespaces(header_parser);

      parser_step = parser_steps[step_index];
      if (!parser_step(header_parser))
        return FALSE;
    }

  return TRUE;
}


static inline gboolean
_expect_char(SnmpTrapdHeaderParser *header_parser, gchar c)
{
  return scan_expect_char(header_parser->input, (gint *) header_parser->input_len, c);
}

static inline gboolean
_expect_newline(SnmpTrapdHeaderParser *header_parser)
{
  return _expect_char(header_parser, '\n');
}

static inline gboolean
_expect_colon(SnmpTrapdHeaderParser *header_parser)
{
  return _expect_char(header_parser, ':');
}

static inline gboolean
_expect_tab(SnmpTrapdHeaderParser *header_parser)
{
  return _expect_char(header_parser, '\t');
}


static gboolean
_parse_v1_uptime(SnmpTrapdHeaderParser *header_parser)
{
  if (!scan_expect_str(header_parser->input, (gint *) header_parser->input_len, "Uptime:"))
    return FALSE;

  _skip_whitespaces(header_parser);

  const gchar *uptime_start = *header_parser->input;

  const gchar *uptime_end = strchr(uptime_start, '\n');
  if (!uptime_end)
    return FALSE;

  log_msg_set_value_by_name(header_parser->msg, _get_formatted_key(header_parser, "uptime"),
                            uptime_start, uptime_end - uptime_start);

  *header_parser->input_len -= uptime_end - *header_parser->input;
  *header_parser->input = uptime_end;
  return TRUE;
}

static gboolean
_parse_v1_trap_type_and_subtype(SnmpTrapdHeaderParser *header_parser)
{
  const gchar *type_start = *header_parser->input;

  const gchar *type_end = strpbrk(type_start, "(\n");
  gboolean type_exists = type_end && *type_end == '(';

  if (!type_exists)
    return FALSE;

  const gchar *subtype_start = type_end + 1;

  if (*(type_end - 1) == ' ')
    --type_end;

  log_msg_set_value_by_name(header_parser->msg, _get_formatted_key(header_parser, "type"),
                            type_start, type_end - type_start);

  const gchar *subtype_end = strpbrk(subtype_start, ")\n");
  gboolean subtype_exists = subtype_end && *subtype_end == ')';

  if (!subtype_exists)
    return FALSE;

  log_msg_set_value_by_name(header_parser->msg, _get_formatted_key(header_parser, "subtype"),
                            subtype_start, subtype_end - subtype_start);


  *header_parser->input_len -= (subtype_end + 1) - *header_parser->input;
  *header_parser->input = subtype_end + 1;
  return TRUE;
}

static gboolean
_parse_v1_enterprise_oid(SnmpTrapdHeaderParser *header_parser)
{
  const gchar *enterprise_string_start = *header_parser->input;
  gsize input_left = *header_parser->input_len;

  while (*header_parser->input_len > 0 && !g_ascii_isspace(**header_parser->input))
    {
      ++(*header_parser->input);
      --(*header_parser->input_len);
    }

  gsize enterprise_string_length = input_left - *header_parser->input_len;

  /* enterprise_string is optional */
  if (enterprise_string_length == 0)
    return TRUE;

  log_msg_set_value_by_name(header_parser->msg, _get_formatted_key(header_parser, "enterprise_oid"),
                            enterprise_string_start, enterprise_string_length);

  return TRUE;
}


static gboolean
_parse_transport_info(SnmpTrapdHeaderParser *header_parser)
{
  if(!scan_expect_char(header_parser->input, (gint *) header_parser->input_len, '['))
    return FALSE;

  _skip_whitespaces(header_parser);

  const gchar *transport_info_start = *header_parser->input;

  const gchar *transport_info_end = strchr(transport_info_start, '\n');
  if (!transport_info_end)
    return FALSE;

  while(*transport_info_end != ']')
    {
      --transport_info_end;
      if(transport_info_end == transport_info_start)
        return FALSE;
    }

  gsize transport_info_len = transport_info_end - transport_info_start;

  log_msg_set_value_by_name(header_parser->msg, _get_formatted_key(header_parser, "transport_info"),
                            transport_info_start, transport_info_len);


  *header_parser->input_len -= (transport_info_end + 1) - *header_parser->input;
  *header_parser->input = transport_info_end + 1;
  return TRUE;
}

static gboolean
_parse_hostname(SnmpTrapdHeaderParser *header_parser)
{
  const gchar *hostname_start = *header_parser->input;
  gsize input_left = *header_parser->input_len;

  while (*header_parser->input_len > 0 && !g_ascii_isspace(**header_parser->input))
    {
      ++(*header_parser->input);
      --(*header_parser->input_len);
    }

  gsize hostname_length = input_left - *header_parser->input_len;
  if (hostname_length == 0)
    return FALSE;

  log_msg_set_value(header_parser->msg, LM_V_HOST, hostname_start, hostname_length);
  return TRUE;
}

static gboolean
_parse_timestamp(SnmpTrapdHeaderParser *header_parser)
{
  GTimeVal now;
  cached_g_current_time(&now);
  time_t now_tv_sec = (time_t) now.tv_sec;

  LogStamp *stamp = &header_parser->msg->timestamps[LM_TS_STAMP];
  stamp->tv_usec = 0;
  stamp->zone_offset = -1;

  /* NOTE: we initialize various unportable fields in tm using a
   * localtime call, as the value of tm_gmtoff does matter but it does
   * not exist on all platforms and 0 initializing it causes trouble on
   * time-zone barriers */

  struct tm tm;
  cached_localtime(&now_tv_sec, &tm);
  if (!scan_std_timestamp(header_parser->input, (gint *)header_parser->input_len, &tm))
    return FALSE;

  stamp->tv_sec = cached_mktime(&tm);
  stamp->zone_offset = get_local_timezone_ofs(stamp->tv_sec);

  return TRUE;
}

static gboolean
_try_parse_v1_info(SnmpTrapdHeaderParser *header_parser)
{
  /* detect v1 format */
  const gchar *new_line = strchr(*header_parser->input, '\n');
  if (new_line && new_line[1] != '\t')
    return TRUE;

  SnmpTrapdHeaderParserStep v1_info_parser_steps[] =
  {
    _parse_v1_enterprise_oid,
    _expect_newline,
    _expect_tab,
    _parse_v1_trap_type_and_subtype,
    _parse_v1_uptime,
    _try_parse_v1_info
  };

  return _run_header_parser(header_parser, v1_info_parser_steps,
                            sizeof(v1_info_parser_steps) / sizeof(SnmpTrapdHeaderParserStep));
}

gboolean
snmptrapd_parse_header(SnmpTrapdHeaderParser *header_parser)
{
  SnmpTrapdHeaderParserStep parser_steps[] =
  {
    _parse_timestamp,
    _parse_hostname,
    _parse_transport_info,
    _expect_colon,
    _try_parse_v1_info,
    _expect_newline
  };

  return _run_header_parser(header_parser, parser_steps, sizeof(parser_steps) / sizeof(SnmpTrapdHeaderParserStep));
}
