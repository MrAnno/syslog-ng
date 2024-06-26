/*
 * Copyright (c) 2024 shifter
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

#include "filterx-func-parse-kv.h"
#include "filterx/object-string.h"
#include "filterx/object-primitive.h"
#include "filterx/expr-literal.h"
#include "filterx/filterx-eval.h"
#include "filterx/filterx-globals.h"
#include "filterx/object-json.h"
#include "filterx/object-message-value.h"
#include "filterx/object-null.h"

#include "kv-parser.h"
#include "parser/parser-expr.h"
#include "scratch-buffers.h"

#define PARSE_KV_OPTS_COUNT 3

enum parse_kv_opts
{
  PARSE_KV_OPTS_VALUE_SEP = 0,
  PARSE_KV_OPTS_PAIR_SEP = 1,
  PARSE_KV_OPTS_STRAY_WORDS_KEY = 2,
};

static const gchar *parse_kv_opts_names[PARSE_KV_OPTS_COUNT] =
{
  "value_separator",
  "pair_separator",
  "stray_words_key"
};

typedef struct FilterXFunctionParseKV_
{
  FilterXFunction super;
  FilterXExpr *msg;
  gchar value_separator;
  gchar *pair_separator;
  gchar *stray_words_key;
} FilterXFunctionParseKV;

static gboolean
_is_valid_separator_character(char c)
{
  return (c != ' '  &&
          c != '\'' &&
          c != '\"' );
}

static void
_set_value_separator(FilterXFunctionParseKV *self, gchar value_separator)
{
  self->value_separator = value_separator;
}

static void
_set_pair_separator(FilterXFunctionParseKV *self, const gchar *pair_separator)
{
  g_free(self->pair_separator);
  self->pair_separator = g_strdup(pair_separator);
}

static void
_set_stray_words_key(FilterXFunctionParseKV *self, const gchar *value_name)
{
  g_free(self->stray_words_key);
  self->stray_words_key = g_strdup(value_name);
}

static gboolean
_apply_parse_kv_option(FilterXFunctionParseKV *self, int opt, const gchar *opt_val, GError **error)
{
  switch (opt)
    {
    case PARSE_KV_OPTS_VALUE_SEP:
    {
      if (strlen(opt_val) < 1)
        {
          g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                      "%s argument can not be empty. " FILTERX_FUNC_PARSE_KV_USAGE,
                      parse_kv_opts_names[opt]);
          return FALSE;
        }
      if (!_is_valid_separator_character(opt_val[0]))
        {
          g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                      "%s argument contains invalid separator character. " FILTERX_FUNC_PARSE_KV_USAGE,
                      parse_kv_opts_names[opt]);
          return FALSE;
        }
      _set_value_separator(self, opt_val[0]);
      break;
    }
    case PARSE_KV_OPTS_PAIR_SEP:
      _set_pair_separator(self, opt_val);
      break;
    case PARSE_KV_OPTS_STRAY_WORDS_KEY:
      _set_stray_words_key(self, opt_val);
      break;
    default:
      g_assert_not_reached();
      break;
    }
  return TRUE;
}

gboolean
_set_json_value(FilterXObject *out,
                const gchar *key, gsize key_len,
                const gchar *value, gsize value_len)
{
  msg_trace("filterx: parse_kv() key-value found",
            evt_tag_str("key", key),
            evt_tag_str("value", value));

  FilterXObject *json_key = filterx_string_new(key, key_len);
  FilterXObject *json_val = filterx_string_new(value, value_len);

  gboolean ok = filterx_object_set_subscript(out, json_key, &json_val);

  filterx_object_unref(json_key);
  filterx_object_unref(json_val);
  return ok;
}

static gboolean
_extract_key_values(FilterXFunctionParseKV *self, const gchar *input, gsize input_len, FilterXObject *output)
{
  KVScanner scanner;
  gboolean result = FALSE;

  kv_scanner_init(&scanner, self->value_separator, self->pair_separator, self->stray_words_key != NULL);
  /* FIXME: input_len is ignored */
  kv_scanner_input(&scanner, input);
  while (kv_scanner_scan_next(&scanner))
    {
      const gchar *name = kv_scanner_get_current_key(&scanner);
      gsize name_len = kv_scanner_get_current_key_len(&scanner);
      const gchar *value = kv_scanner_get_current_value(&scanner);
      gsize value_len = kv_scanner_get_current_value_len(&scanner);

      if (!_set_json_value(output, name, name_len, value, value_len))
        goto exit;
    }

  if (self->stray_words_key &&
      !_set_json_value(output, self->stray_words_key, -1,
                       kv_scanner_get_stray_words(&scanner), kv_scanner_get_stray_words_len(&scanner)))
    goto exit;

  result = TRUE;
exit:
  kv_scanner_deinit(&scanner);
  return result;
}

static FilterXObject *
_eval(FilterXExpr *s)
{
  FilterXFunctionParseKV *self = (FilterXFunctionParseKV *) s;

  FilterXObject *obj = filterx_expr_eval(self->msg);
  if (!obj)
    return NULL;

  gsize len;
  const gchar *input;
  FilterXObject *result = NULL;

  if (filterx_object_is_type(obj, &FILTERX_TYPE_NAME(string)))
    input = filterx_string_get_value(obj, &len);
  else if (filterx_object_is_type(obj, &FILTERX_TYPE_NAME(message_value)))
    input = filterx_message_value_get_value(obj, &len);
  else
    goto exit;

  result = filterx_json_object_new_empty();
  if (!_extract_key_values(self, input, len, result))
    {
      filterx_object_unref(result);
      result = NULL;
    }
exit:
  filterx_object_unref(obj);
  return result;
}

static void
_free(FilterXExpr *s)
{
  FilterXFunctionParseKV *self = (FilterXFunctionParseKV *) s;
  filterx_expr_unref(self->msg);
  g_free(self->pair_separator);
  filterx_function_free_method(&self->super);
}

static FilterXExpr *
_extract_parse_kv_msg_expr(GList *argument_expressions, GError **error)
{
  FilterXExpr *msg_expr = filterx_expr_ref(((FilterXExpr *) argument_expressions->data));
  if (!msg_expr)
    {
      g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                  "argument must be set: msg_str. " FILTERX_FUNC_PARSE_KV_USAGE);
      return NULL;
    }

  return msg_expr;
}

static gboolean
_extract_parse_kv_opts(FilterXFunctionParseKV *self, GList *argument_expressions, GError **error)
{
  gsize arguments_len = argument_expressions ? g_list_length(argument_expressions) : 0;
  if (arguments_len < 1)
    {
      g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                  "invalid number of arguments. " FILTERX_FUNC_PARSE_KV_USAGE);
      return FALSE;
    }

  FilterXObject *format_obj = NULL;
  guint32 i = 0;
  for (GList *elem = argument_expressions->next; elem; elem = elem->next)
    {
      if (i >= PARSE_KV_OPTS_COUNT)
        break;

      FilterXExpr *argument_expr = (FilterXExpr *) elem->data;

      if (!argument_expr || !filterx_expr_is_literal(argument_expr))
        {
          g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                      "%s argument must be string literal. " FILTERX_FUNC_PARSE_KV_USAGE,
                      parse_kv_opts_names[i]);
          return FALSE;
        }

      format_obj = filterx_expr_eval(argument_expr);
      if (!format_obj)
        {
          g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                      "%s argument. " FILTERX_FUNC_PARSE_KV_USAGE, parse_kv_opts_names[i]);
          goto error;
        }

      if (filterx_object_is_type(format_obj, &FILTERX_TYPE_NAME(string)))
        {
          gsize format_len;
          const gchar *opt = filterx_string_get_value(format_obj, &format_len);

          if (!opt)
            {
              g_set_error(error, FILTERX_FUNCTION_ERROR, FILTERX_FUNCTION_ERROR_CTOR_FAIL,
                          "%s argument must be string literal. " FILTERX_FUNC_PARSE_KV_USAGE,
                          parse_kv_opts_names[i]);
              goto error;
            }

          if (!_apply_parse_kv_option(self, i, opt, error))
            goto error;
        }

      filterx_object_unref(format_obj);
      i++;
    }

  return TRUE;

error:
  filterx_object_unref(format_obj);
  return FALSE  ;
}

FilterXExpr *
filterx_function_parse_kv_new(const gchar *function_name, GList *argument_expressions, GError **error)

{
  FilterXFunctionParseKV *self = g_new0(FilterXFunctionParseKV, 1);
  filterx_function_init_instance(&self->super, function_name);
  self->super.super.eval = _eval;
  self->super.super.free_fn = _free;
  self->value_separator = '=';
  self->pair_separator = g_strdup(", ");

  if (!_extract_parse_kv_opts(self, argument_expressions, error))
    goto error;

  self->msg = _extract_parse_kv_msg_expr(argument_expressions, error);
  if (!self->msg)
    goto error;

  g_list_free_full(argument_expressions, (GDestroyNotify) filterx_expr_unref);
  return &self->super.super;

error:
  g_list_free_full(argument_expressions, (GDestroyNotify) filterx_expr_unref);
  filterx_expr_unref(&self->super.super);
  return NULL;
}

gpointer
filterx_function_construct_parse_kv(Plugin *self)
{
  return (gpointer) filterx_function_parse_kv_new;
}
