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
#include "snmptrapd-nv-context.h"
#include "varbindlist-scanner.h"
#include "scratch-buffers2.h"
#include "utf8utils.h"

typedef struct _SnmpTrapdParser
{
  LogParser super;
  GString *prefix;

  GString *formatted_key;
  gboolean generate_message;
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

void
snmptrapd_parser_set_generate_message(LogParser *s, gboolean generate_message)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  self->generate_message = generate_message;
}

static inline gboolean
_is_unwanted_key_char(gchar c)
{
  return (c == ':');
}

static inline void
_move_and_replace_unwanted_key_chars(GString *key, gsize unwanted_key_count, gchar **curr_char_addr)
{
  gsize number_of_removed_chars = unwanted_key_count - 1;
  gchar *curr_pos = *curr_char_addr;
  gchar *end_of_str = key->str + key->len;
  gchar *replace_pos = curr_pos - unwanted_key_count;

  if (unwanted_key_count > 1)
    {
      memmove(replace_pos, curr_pos - 1, end_of_str - (curr_pos - 1));
      g_string_truncate(key, key->len - number_of_removed_chars);
    }
  *replace_pos = '_';
  *curr_char_addr -= unwanted_key_count;
}

static const gchar *
_normalize_key(GString *key)
{
  gsize unwanted_key_count = 0;
  gchar *curr_char = key->str;
  while (*curr_char)
    {
      if (_is_unwanted_key_char(*curr_char))
        {
          unwanted_key_count++;
        }
      else if (unwanted_key_count > 0)
        {
          _move_and_replace_unwanted_key_chars(key, unwanted_key_count, &curr_char);
          unwanted_key_count = 0;
        }
      curr_char++;
    }
  if (unwanted_key_count)
    _move_and_replace_unwanted_key_chars(key, unwanted_key_count, &curr_char);

  return key->str;
}

static const gchar *
_get_formatted_key(const gchar *key, const GString *prefix, GString *formatted_key)
{
  g_string_truncate(formatted_key, 0);

  if (prefix->len > 0)
    g_string_assign(formatted_key, prefix->str);

  g_string_append(formatted_key, key);

  _normalize_key(formatted_key);

  return formatted_key->str;
}

static void
_append_name_value_to_generated_message(GString *generated_message, const gchar *key,
                                        const gchar *value, gsize value_length)
{
  ScratchBuffersMarker marker;
  GString *escaped_value = scratch_buffers2_alloc_and_mark(&marker);

  if (generated_message->len > 0)
    g_string_append(generated_message, ", ");

  append_unsafe_utf8_as_escaped_text(escaped_value, value, value_length, "'");
  g_string_append_printf(generated_message, "%s='%s'", key, escaped_value->str);

  scratch_buffers2_reclaim_marked(marker);
}

static void
_add_name_value(SnmpTrapdNVContext *nv_context, const gchar *key,
                const gchar *value, gsize value_length)
{
  ScratchBuffersMarker marker;
  GString *formatted_key = scratch_buffers2_alloc_and_mark(&marker);

  const gchar *prefixed_key = _get_formatted_key(key, nv_context->key_prefix, formatted_key);
  log_msg_set_value_by_name(nv_context->msg, prefixed_key, value, value_length);

  if (nv_context->generated_message)
    _append_name_value_to_generated_message(nv_context->generated_message, key, value, value_length);

  scratch_buffers2_reclaim_marked(marker);
}

static gboolean
_parse_varbindlist(SnmpTrapdNVContext *nv_context, const gchar **input, gsize *input_len)
{
  VarBindListScanner varbindlist_scanner;
  const gchar *key, *value;

  varbindlist_scanner_init(&varbindlist_scanner);

  varbindlist_scanner_input(&varbindlist_scanner, *input);
  while (varbindlist_scanner_scan_next(&varbindlist_scanner))
    {
      key = varbindlist_scanner_get_current_key(&varbindlist_scanner);
      value = varbindlist_scanner_get_current_value(&varbindlist_scanner);

      snmptrapd_nv_context_add_name_value(nv_context, key, value, -1);
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

  ScratchBuffersMarker marker;

  GString *generated_message = NULL;
  if (self->generate_message)
    generated_message = scratch_buffers2_alloc_and_mark(&marker);


  SnmpTrapdNVContext nv_context =
  {
    .key_prefix = self->prefix,
    .msg = *pmsg,
    .generated_message = generated_message,
    .add_name_value = _add_name_value
  };

  log_msg_set_value(nv_context.msg, LM_V_PROGRAM, "snmptrapd", -1);

  if (!snmptrapd_header_parser_parse(&nv_context, &input, &input_len))
    return FALSE;

  if (!_parse_varbindlist(&nv_context, &input, &input_len))
    return FALSE;


  if (self->generate_message)
    {
      log_msg_set_value(nv_context.msg, LM_V_MESSAGE, nv_context.generated_message->str, -1);
      scratch_buffers2_reclaim_marked(marker);
    }
  else
    {
      log_msg_unset_value(nv_context.msg, LM_V_MESSAGE);
    }

  return TRUE;
}

static LogPipe *
snmptrapd_parser_clone(LogPipe *s)
{
  SnmpTrapdParser *self = (SnmpTrapdParser *) s;

  SnmpTrapdParser *cloned = (SnmpTrapdParser *) snmptrapd_parser_new(s->cfg);

  snmptrapd_parser_set_prefix(&cloned->super, self->prefix->str);
  snmptrapd_parser_set_generate_message(&cloned->super, self->generate_message);

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
  self->generate_message = TRUE;

  return &self->super;
}
