#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include "msg_parse_lib.h"

#include "syslog-ng.h"
#include "logmsg/logmsg.h"
#include "serialize.h"
#include "apphook.h"
#include "gsockaddr.h"
#include "logpipe.h"
#include "cfg.h"
#include "plugin.h"

#include <string.h>

void
assert_new_log_message_attributes(LogMessage *log_message)
{
  assert_log_message_value(log_message, LM_V_HOST, "newhost");
  assert_log_message_value(log_message, LM_V_HOST_FROM, "newhost");
  assert_log_message_value(log_message, LM_V_MESSAGE, "newmsg");
  assert_log_message_value(log_message, LM_V_PROGRAM, "newprogram");
  assert_log_message_value(log_message, LM_V_PID, "newpid");
  assert_log_message_value(log_message, LM_V_MSGID, "newmsgid");
  assert_log_message_value(log_message, LM_V_SOURCE, "newsource");
  assert_log_message_value(log_message, log_msg_get_value_handle("newvalue"), "newvalue");
}

void
set_new_log_message_attributes(LogMessage *log_message)
{
  log_msg_set_value(log_message, LM_V_HOST, "newhost", -1);
  log_msg_set_value(log_message, LM_V_HOST_FROM, "newhost", -1);
  log_msg_set_value(log_message, LM_V_MESSAGE, "newmsg", -1);
  log_msg_set_value(log_message, LM_V_PROGRAM, "newprogram", -1);
  log_msg_set_value(log_message, LM_V_PID, "newpid", -1);
  log_msg_set_value(log_message, LM_V_MSGID, "newmsgid", -1);
  log_msg_set_value(log_message, LM_V_SOURCE, "newsource", -1);
  log_msg_set_value_by_name(log_message, "newvalue", "newvalue", -1);
}


/*
 * Tests
 */

void
setup(void)
{
  app_startup();
  init_and_load_syslogformat_module();
}

void teardown(void)
{
  deinit_syslogformat_module();
  app_shutdown();
}

TestSuite(clone_logmsg, .init = setup, .fini = teardown);

ParameterizedTestParameters(clone_logmsg, test_cloning_with_log_message)
{
  const gchar *messages[] = {
    "<7>1 2006-10-29T01:59:59.156+01:00 mymachine.example.com evntslog - ID47 [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] BOMAn application event log entry...",
    "<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog - - [exampleSDID@0 iut=\"3\"] [eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] BOMAn application event log entry...",
  };

  // generate parameter set
  return cr_make_param_array(const gchar *, messages, sizeof(messages) / sizeof(const gchar *));
}

ParameterizedTest(const gchar *msg, clone_logmsg, test_cloning_with_log_message)
{
  LogMessage *original_log_message, *log_message, *cloned_log_message;
  regex_t bad_hostname;
  GSockAddr *addr = g_sockaddr_inet_new("10.10.10.10", 1010);
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;

  parse_options.flags = LP_SYSLOG_PROTOCOL;
  parse_options.bad_hostname = &bad_hostname;

  original_log_message = log_msg_new(msg, strlen(msg), addr, &parse_options);
  log_message = log_msg_new(msg, strlen(msg), addr, &parse_options);

  log_msg_set_tag_by_name(log_message, "newtag");
  path_options.ack_needed = FALSE;

  cloned_log_message = log_msg_clone_cow(log_message, &path_options);
  assert_log_messages_equal(cloned_log_message, original_log_message);

  set_new_log_message_attributes(cloned_log_message);

  assert_log_messages_equal(log_message, original_log_message);
  assert_new_log_message_attributes(cloned_log_message);
  assert_log_message_has_tag(cloned_log_message, "newtag");

  log_msg_unref(cloned_log_message);
  log_msg_unref(log_message);
  log_msg_unref(original_log_message);
}
