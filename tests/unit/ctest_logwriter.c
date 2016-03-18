#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include "syslog-ng.h"
#include "logwriter.h"
#include "logmsg/logmsg.h"
#include "template/templates.h"
#include "apphook.h"
#include "cfg.h"
#include "timeutils.h"
#include "plugin.h"
#include "logqueue-fifo.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

MsgFormatOptions parse_options;

LogMessage *
init_msg(const gchar *msg_string, gboolean use_syslog_protocol)
{
  LogMessage *msg;
  GSockAddr *sa;

  if (use_syslog_protocol)
    parse_options.flags |= LP_SYSLOG_PROTOCOL;
  else
    parse_options.flags &= ~LP_SYSLOG_PROTOCOL;
  sa = g_sockaddr_inet_new("10.10.10.10", 1010);
  msg = log_msg_new(msg_string, strlen(msg_string), sa, &parse_options);
  g_sockaddr_unref(sa);
  log_msg_set_value_by_name(msg, "APP.VALUE", "value", 5);
  log_msg_set_match(msg, 0, "whole-match", 11);
  log_msg_set_match(msg, 1, "first-match", 11);

  /* fix some externally or automatically defined values */
  log_msg_set_value(msg, LM_V_HOST_FROM, "kismacska", 9);
  msg->timestamps[LM_TS_RECVD].tv_sec = 1139684315;
  msg->timestamps[LM_TS_RECVD].tv_usec = 639000;
  msg->timestamps[LM_TS_RECVD].zone_offset = get_local_timezone_ofs(1139684315);
  return msg;
}


/*
 * Tests
 */

struct logwriter_params {
  const gchar *msg_string;
  gboolean input_is_rfc5424;
  gchar *template;
  guint writer_flags;
  const gchar *expected_value;
};

void
setup(void)
{
  configuration = cfg_new(0x0300);
  app_startup();
  putenv("TZ=MET-1METDST");
  tzset();

  plugin_load_module("syslogformat", configuration, NULL);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, configuration);
}

void
teardown(void)
{
  app_shutdown();
}

TestSuite(logwriter, .init = setup, .fini = teardown);

ParameterizedTestParameters(logwriter, test_logwriter) {
  const gchar *msg_syslog_str = "<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] BOMAn application event log entry...";
  const gchar *expected_msg_syslog_str = "<132>1 2006-10-29T01:59:59+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] BOMAn application event log entry...\n";
  const gchar *expected_msg_syslog_str_t = "<132>1 2006-10-29T01:59:59+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] ID47 BOMAn application event log entry...\n";
  const gchar *expected_msg_syslog_to_bsd_str = "<132>Oct 29 01:59:59 mymachine evntslog[3535]: BOMAn application event log entry...\n";
  const gchar *expected_msg_syslog_to_file_str = "Oct 29 01:59:59 mymachine evntslog[3535]: BOMAn application event log entry...\n";
  const gchar *msg_syslog_empty_str ="<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"]";
  const gchar *expected_msg_syslog_empty_str ="<132>1 2006-10-29T01:59:59+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] \n";
  const gchar *expected_msg_syslog_empty_str_t ="<132>1 2006-10-29T01:59:59+01:00 mymachine evntslog 3535 ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] ID47\n";
  const gchar *msg_bsd_str = "<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]:árvíztűrőtükörfúrógép";
  const gchar *expected_msg_bsd_str = "Feb 11 10:34:56 bzorp syslog-ng[23323]:árvíztűrőtükörfúrógép\n";
  const gchar *expected_msg_bsd_str_t = "155 23323 árvíztűrőtükörfúrógép";
  const gchar *expected_msg_bsd_to_syslog_str = "<155>1 2006-02-11T10:34:56+01:00 bzorp syslog-ng 23323 - - árvíztűrőtükörfúrógép\n";
  const gchar *expected_msg_bsd_to_proto_str = "<155>Feb 11 10:34:56 bzorp syslog-ng[23323]:árvíztűrőtükörfúrógép\n";
  const gchar *msg_zero_pri = "<0>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]:árvíztűrőtükörfúrógép";
  const gchar *expected_msg_zero_pri_str = "<0>Feb 11 10:34:56 bzorp syslog-ng[23323]:árvíztűrőtükörfúrógép\n";
  const gchar *expected_msg_zero_pri_str_t = "0";
  const gchar *msg_bsd_empty_str = "<155>2006-02-11T10:34:56+01:00 bzorp syslog-ng[23323]:";
  const gchar *expected_msg_bsd_empty_str = "<155>Feb 11 10:34:56 bzorp syslog-ng[23323]:\n";
  const gchar *expected_msg_bsd_empty_str_t = "23323";

  struct logwriter_params params[] = {
    {msg_syslog_str, TRUE, NULL, LW_SYSLOG_PROTOCOL, expected_msg_syslog_str},
    {msg_syslog_str, TRUE, "$MSGID $MSG", LW_SYSLOG_PROTOCOL, expected_msg_syslog_str_t},
    {msg_syslog_empty_str, TRUE, NULL, LW_SYSLOG_PROTOCOL, expected_msg_syslog_empty_str},
    {msg_syslog_empty_str, TRUE, "$MSGID", LW_SYSLOG_PROTOCOL, expected_msg_syslog_empty_str_t},
    {msg_syslog_str, TRUE, NULL, LW_FORMAT_PROTO, expected_msg_syslog_to_bsd_str},
    {msg_syslog_str, TRUE, NULL, LW_FORMAT_FILE, expected_msg_syslog_to_file_str},
    {msg_bsd_str, FALSE, NULL, LW_FORMAT_FILE, expected_msg_bsd_str},
    {msg_bsd_str, FALSE, "$PRI $PID $MSG", LW_FORMAT_FILE, expected_msg_bsd_str_t},
    {msg_bsd_str, FALSE, NULL, LW_FORMAT_PROTO, expected_msg_bsd_to_proto_str},
    {msg_bsd_str, FALSE, NULL, LW_SYSLOG_PROTOCOL, expected_msg_bsd_to_syslog_str},
    {msg_bsd_empty_str, FALSE, NULL, LW_FORMAT_PROTO, expected_msg_bsd_empty_str},
    {msg_bsd_empty_str, FALSE, "$PID", LW_FORMAT_PROTO, expected_msg_bsd_empty_str_t},
    {msg_zero_pri, FALSE, NULL, LW_FORMAT_PROTO, expected_msg_zero_pri_str},
    {msg_zero_pri, FALSE, "$PRI", LW_FORMAT_PROTO, expected_msg_zero_pri_str_t}
  };

  return cr_make_param_array(struct logwriter_params, params, sizeof (params) / sizeof(struct logwriter_params));
}

ParameterizedTest(struct logwriter_params *params, logwriter, test_logwriter)
{
  LogTemplate *templ = NULL;
  LogWriter *writer = NULL;
  GString *res = g_string_sized_new(128);
  GError *error = NULL;
  LogMessage *msg = NULL;
  LogWriterOptions opt = {0};
  LogQueue *queue;

  static TimeZoneInfo *tzinfo = NULL;

  if (!tzinfo)
    tzinfo = time_zone_info_new(NULL);
  opt.options = LWO_NO_MULTI_LINE | LWO_NO_STATS | LWO_SHARE_STATS;
  opt.template_options.time_zone_info[LTZ_SEND]=tzinfo;

  if (params->template)
    {
      templ = log_template_new(configuration, "dummy");
      log_template_compile(templ, params->template, &error);
    }
  opt.template = templ;
  msg = init_msg(params->msg_string, params->input_is_rfc5424);
  queue = log_queue_fifo_new(1000, NULL);
  writer = log_writer_new(params->writer_flags, configuration);

  log_writer_set_options(writer, NULL, &opt, 0, 0, NULL, NULL);
  log_writer_set_queue(writer, queue);
  log_writer_format_log(writer,msg,res);

  cr_assert_str_eq(res->str, params->expected_value,
    "Testcase failed; result: %s, expected: %s",res->str,params->expected_value);

  if (templ)
    log_template_unref(templ);
  log_pipe_unref((LogPipe *) writer);
  log_msg_unref(msg);
  log_queue_unref(queue);
  g_string_free(res, TRUE);
}
