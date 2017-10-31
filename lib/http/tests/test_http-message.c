/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 László Várady
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include <criterion/criterion.h>

#include "http/http-message.h"

Test(http_message, test_headers)
{
  HTTPRequest *request = http_request_new_empty();
  HTTPMessage *message = http_request_upcast(request);
  request = NULL;

  const gchar *accept_key = "Accept";
  const gchar *accept_value = "text/html,application/xhtml+xml,application/xml;q=0.9";

  const gchar *accept_enc_key = "Accept-Encoding";
  const gchar *accept_enc_value = "gzip, deflate, br";

  http_message_add_header(message, accept_key, accept_value);
  http_message_add_header(message, accept_enc_key, accept_enc_value);

  GString *actual_value = http_message_get_header(message, accept_key);
  cr_assert_not_null(actual_value);
  cr_assert_str_eq(actual_value->str, accept_value);
  g_string_free(actual_value, TRUE);

  actual_value = http_message_get_header(message, accept_enc_key);
  cr_assert_not_null(actual_value);
  cr_assert_str_eq(actual_value->str, accept_enc_value);
  g_string_free(actual_value, TRUE);

  actual_value = http_message_get_header_using_normalized_key(message, "accept-encoding");
  cr_assert_not_null(actual_value);
  cr_assert_str_eq(actual_value->str, accept_enc_value);
  g_string_free(actual_value, TRUE);

  http_message_free(message);
}