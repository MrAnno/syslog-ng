#include <gtest/gtest.h>

extern "C" {
  // note: Viktor is always right.
  #define new mynew
  #define template mytemplate
  #include "logmsg.h"
  #include "apphook.h"
  #include "cfg.h"
  #include "plugin.h"
  #include "msg_parse_lib.h"
  #include "testutils.h"
  #undef new
  #undef mytemplate
}

#include <stdlib.h>
#include <string.h>

namespace {

class SyslogNGTest : public ::testing::Test {
protected:
  SyslogNGTest()
  {
    // You can do set-up work for each test here.
  }

  virtual ~SyslogNGTest()
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  virtual void SetUp()
  {
    ::app_startup();
    ::init_and_load_syslogformat_module();
  }

  virtual void TearDown()
  {
    ::deinit_syslogformat_module();
    ::app_shutdown();
  }

  void testcase_update_sdata(const gchar *msg, const gchar *expected_sd_str, gchar *elem_name1, ...)
  {
    LogMessage *logmsg;
    GString *sd_str = g_string_new("");
    va_list va;
    gchar *elem, *param, *value;

    parse_options.flags |= LP_SYSLOG_PROTOCOL;

    va_start(va, elem_name1);

    logmsg = log_msg_new(msg, strlen(msg), NULL, &parse_options);

    elem = elem_name1;
    param = va_arg(va, char *);
    value = va_arg(va, char *);
    while (elem)
      {
        gchar sd_name[64];

        g_snprintf(sd_name, sizeof(sd_name), ".SDATA.%s.%s", elem, param);
        log_msg_set_value_by_name(logmsg, sd_name, value, -1);
        elem = va_arg(va, char *);
        param = va_arg(va, char *);
        value = va_arg(va, char *);
      }

    log_msg_format_sdata(logmsg, sd_str, 0);

    EXPECT_STREQ(sd_str->str, expected_sd_str) << "sdata update failed";

    g_string_free(sd_str, TRUE);
    log_msg_unref(logmsg);
  }
};

TEST_F(SyslogNGTest, test_msg_sdata)
{
  testcase_update_sdata("<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog - - [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] An application event log entry...",
                  "[exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"][meta sequenceId=\"11\"][syslog-ng param=\"value\"]",
                  "meta", "sequenceId", "11",
                  "syslog-ng", "param", "value",
                  NULL, NULL, NULL);
}

}  // namespace
