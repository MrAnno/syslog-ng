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
 *
 */

#include "varbindlist-scanner.h"
#include "str-utils.h"

static inline gboolean
_is_valid_key_character(gchar c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '_') ||
         (c == '.') ||
         (c == '-') ||
         (c == ':') ||
         (c == '"');
}

static KVScanner *
_clone(KVScanner *s)
{
  VarBindListScanner *scanner = varbindlist_scanner_new();
  return &scanner->super;
}

static void
_free(KVScanner *s)
{
  VarBindListScanner *self = (VarBindListScanner *) s;

  kv_scanner_free_method(&self->super);
  g_string_free(self->varbind_type, TRUE);
}

static inline void
_skip_whitespaces(const gchar **input)
{
  const gchar *current_char = *input;

  while (g_ascii_isspace(*current_char))
    ++current_char;

  *input = current_char;
}

static const gchar *
_extract_type(VarBindListScanner *self)
{
  const gchar *type_start = kv_scanner_get_current_value(&self->super);
  const gchar *type_end = strchr(type_start, ':');

  if (type_end)
    {
      gsize type_len = type_end - type_start;
      g_string_assign_len(self->varbind_type, type_start, type_len);
      ++type_end;
    }
  else
    {
      /* no type */
      type_end = type_start;
      g_string_truncate(self->varbind_type, 0);
    }

  return type_end;
}

static gboolean
_extract_type_and_value(KVScanner *scanner)
{
  VarBindListScanner *self = (VarBindListScanner *) scanner;

  const gchar *value_start = _extract_type(self);
  _skip_whitespaces(&value_start);

  g_string_assign(scanner->decoded_value, value_start);
  return TRUE;
}

gboolean
varbindlist_scanner_scan_next(VarBindListScanner *self)
{
  return kv_scanner_scan_next(&self->super);
}

VarBindListScanner *
varbindlist_scanner_new(void)
{
  VarBindListScanner *self = g_new0(VarBindListScanner, 1);
  kv_scanner_init_instance(&self->super, '=', "\t", TRUE);
  kv_scanner_set_transform_value(&self->super, _extract_type_and_value);
  kv_scanner_set_valid_key_character_func(&self->super, _is_valid_key_character);

  self->varbind_type = g_string_sized_new(16);
  self->super.clone = _clone;
  self->super.free_fn = _free;

  return self;
}
