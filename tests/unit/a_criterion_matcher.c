#include <criterion/criterion.h>

#include "logmatcher.h"
#include "apphook.h"
#include "plugin.h"
#include "cfg.h"

#include <stdlib.h>
#include <string.h>

static MsgFormatOptions parse_options;

static LogMessage*
create_log_message(const gchar *log)
{
  LogMessage *msg;
  gchar buf[1024];
  NVHandle nonasciiz = log_msg_get_value_handle("NON-ASCIIZ");
  gssize msglen;
  GSockAddr *sa;

  sa = g_sockaddr_inet_new("10.10.10.10", 1010);
  msg = log_msg_new(log, strlen(log), sa, &parse_options);
  g_sockaddr_unref(sa);

  /* NOTE: we test how our matchers cope with non-zero terminated values. We don't change message_len, only the value */
  g_snprintf(buf, sizeof(buf), "%sAAAAAAAAAAAA", log_msg_get_value(msg, LM_V_MESSAGE, &msglen));
  log_msg_set_value_by_name(msg, "MESSAGE2", buf, -1);

  /* add a non-zero terminated indirect value which contains the whole message */
  log_msg_set_value_indirect(msg, nonasciiz, log_msg_get_value_handle("MESSAGE2"), 0, 0, msglen);

  return msg;
}

static void
testcase_match(const gchar *log, const gchar *pattern, gboolean expected_result, LogMatcher *m)
{
  gboolean result;
  gssize msglen;
  const gchar *value;
  NVHandle nonasciiz = log_msg_get_value_handle("NON-ASCIIZ");

  LogMessage *msg = create_log_message(log);
  log_matcher_compile(m, pattern, NULL);

  value = log_msg_get_value(msg, nonasciiz, &msglen);
  result = log_matcher_match(m, msg, nonasciiz, value, msglen);

  cr_assert_eq(result, expected_result,
                "pattern=%s, result=%d, expected=%d\n",
                pattern, result, expected_result);

  log_matcher_unref(m);
  log_msg_unref(msg);
}

static void
testcase_replace(const gchar *log, const gchar *re, gchar *replacement, const gchar *expected_result, LogMatcher *m)
{
  LogTemplate *r;
  gchar *result;
  gssize length;
  gssize msglen;
  NVHandle nonasciiz = log_msg_get_value_handle("NON-ASCIIZ");
  const gchar *value;

  LogMessage *msg = create_log_message(log);

  log_matcher_compile(m, re, NULL);

  r = log_template_new(configuration, NULL);
  log_template_compile(r, replacement, NULL);

  NVTable *nv_table = nv_table_ref(msg->payload);
  value = log_msg_get_value(msg, nonasciiz, &msglen);
  result = log_matcher_replace(m, msg, nonasciiz, value, msglen, r, &length);
  value = log_msg_get_value(msg, nonasciiz, &msglen);
  nv_table_unref(nv_table);

  cr_assert_arr_eq((result ? result : value), expected_result, (result ? length : msglen),
                    "pattern=%s, result=%.*s, expected=%s\n",
                    re, length, result ? result : value, expected_result);

  g_free(result);

  log_template_unref(r);
  log_matcher_unref(m);
  log_msg_unref(msg);
}

static LogMatcher *
construct_matcher(gint matcher_flags, LogMatcher *(*construct)(const LogMatcherOptions *options))
{
  LogMatcherOptions matcher_options;

  log_matcher_options_defaults(&matcher_options);
  matcher_options.flags = matcher_flags;

  return construct(&matcher_options);
}

void
setup(void)
{
  app_startup();

  configuration = cfg_new(0x0302);
  plugin_load_module("syslogformat", configuration, NULL);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, configuration);
}

void
teardown(void)
{
  msg_format_options_destroy(&parse_options);
  app_shutdown();
}

TestSuite(matcher_suite, .init = setup, .fini = teardown);

Test(matcher_suite, posix_regexp, .description = "POSIX regexp") 
{
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "árvíz", "favíz", "favíztűrőtükörfúrógép", construct_matcher(0, log_matcher_posix_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "^tűrő", "faró", "árvíztűrőtükörfúrógép", construct_matcher(0, log_matcher_posix_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "tűrő", "", "árvíztükörfúrógép", construct_matcher(0, log_matcher_posix_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "wi", "", "kiki", construct_matcher(LMF_GLOBAL, log_matcher_posix_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "wi", "kuku", "kukukikukuki", construct_matcher(LMF_GLOBAL, log_matcher_posix_re_new));
}

Test(matcher_suite, back_ref, .description = "back references are not portable, they work only on Linux")
{
#if __linux__
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "(wiki)\\1", "", "", construct_matcher(LMF_STORE_MATCHES, log_matcher_posix_re_new));
#endif
}

Test(matcher_suite, empty_global, .description = "empty match with global flag")
{
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa bb", "c*", "#", "#a#a# #b#b#", construct_matcher(LMF_GLOBAL, log_matcher_posix_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa bb", "a*", "#", "# #b#b#", construct_matcher(LMF_GLOBAL, log_matcher_posix_re_new));

  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa bb", "c*", "#", "#a#a# #b#b#", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa bb", "a*", "?", "?? ?b?b?", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa", "aa|b*", "@", "@@", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa", "aa|b*", "@", "@", construct_matcher(0, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa", "b*|aa", "@", "@@@", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: aa", "b*|aa", "@", "@aa", construct_matcher(0, log_matcher_pcre_re_new));

  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "wi", "", "kiki", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "wi", "kuku", "kukukikukuki", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
}

Test(matcher_suite, string_match)
{
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "árvíz", "favíz", "favíztűrőtükörfúrógép", construct_matcher(LMF_PREFIX, log_matcher_string_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "tűrő", "faró", "árvízfarótükörfúrógép", construct_matcher(LMF_SUBSTRING, log_matcher_string_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "tűrő", "", "árvíztükörfúrógép", construct_matcher(LMF_SUBSTRING, log_matcher_string_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "árvíztűrőtükörfúrógép", "almafa", "almafa", construct_matcher(0, log_matcher_string_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: ", "valami-amivel-nem-szabadna-matchelni", "almafa", "", construct_matcher(0, log_matcher_string_new));


  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: val", "valami-amivel-nem-szabadna-matchelni", FALSE, construct_matcher(0, log_matcher_string_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: ", "valami-amivel-nem-szabadna-matchelni", FALSE, construct_matcher(0, log_matcher_string_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: ", "valami-amivel-nem-szabadna-matchelni", 0, construct_matcher(LMF_PREFIX, log_matcher_string_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: ", "valami-amivel-nem-szabadna-matchelni", 0, construct_matcher(LMF_SUBSTRING, log_matcher_string_new));

  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: match", "match", TRUE, construct_matcher(0, log_matcher_string_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: match", "ma", TRUE, construct_matcher(LMF_PREFIX, log_matcher_string_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: match", "tch", TRUE, construct_matcher(LMF_SUBSTRING, log_matcher_string_new));

  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: abcdef", "ABCDEF", "qwerty", "qwerty", construct_matcher(LMF_PREFIX | LMF_ICASE, log_matcher_string_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: abcdef", "BCD", "qwerty", "aqwertyef", construct_matcher(LMF_SUBSTRING | LMF_ICASE, log_matcher_string_new));
}

Test(matcher_suite, glob_match)
{
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "árvíz*", TRUE, construct_matcher(0, log_matcher_glob_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "*fúrógép", TRUE, construct_matcher(0, log_matcher_glob_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "*fúró*", TRUE, construct_matcher(0, log_matcher_glob_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "tükör", FALSE, construct_matcher(0, log_matcher_glob_new));
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "viziló", FALSE, construct_matcher(0, log_matcher_glob_new));
}

Test(matcher_suite, iso88592_never, .description = "match in iso-8859-2 never matches")
{
  testcase_match("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: \xe1rv\xedzt\xfbr\xf5t\xfck\xf6rf\xfar\xf3g\xe9p", "\xe1rv\xed*", FALSE, construct_matcher(0, log_matcher_glob_new));
}

Test(matcher_suite, replace)
{
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "árvíz", "favíz", "favíztűrőtükörfúrógép", construct_matcher(0, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "^tűrő", "faró", "árvíztűrőtükörfúrógép", construct_matcher(0, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: árvíztűrőtükörfúrógép", "tűrő", "", "árvíztükörfúrógép", construct_matcher(0, log_matcher_pcre_re_new));
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "(wiki)\\1", "", "", construct_matcher(0, log_matcher_pcre_re_new));
  /* back ref with perl style $1 */
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "(wiki).+", "#$1#", "#wiki#", construct_matcher(0, log_matcher_pcre_re_new));
}

Test(matcher_suite, pcre812_incompatibility, .description = "tests a pcre 8.12 incompatibility")
{
  testcase_replace("<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]: wikiwiki", "([[:digit:]]{1,3}\\.){3}[[:digit:]]{1,3}", "foo", "wikiwiki", construct_matcher(LMF_GLOBAL, log_matcher_pcre_re_new));
}
