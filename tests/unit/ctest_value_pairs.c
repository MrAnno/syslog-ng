#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include "value-pairs/value-pairs.h"
#include "logmsg/logmsg.h"
#include "apphook.h"
#include "cfg.h"
#include "plugin.h"

#include <stdlib.h>
#include <string.h>

#define TEST_KEY "test.key"

gboolean
_vp_keys_foreach(const gchar *name, TypeHint type, const gchar *value,
                gsize value_len, gpointer user_data)
{
  gpointer *args = (gpointer *) user_data;
  GList **keys = (GList **) args[0];
  gboolean *test_key_found = (gboolean *) args[1];

  if (strcmp(name, TEST_KEY) != 0)
    *keys = g_list_insert_sorted(*keys, g_strdup(name), (GCompareFunc) strcmp);
  else
    *test_key_found = TRUE;
  return FALSE;
}

void
_cat_keys_foreach(const gchar *name, gpointer user_data)
{
  GString *res = (GString *) user_data;

  if (res->len > 0)
    g_string_append_c(res, ',');
  g_string_append(res, name);
}

MsgFormatOptions parse_options;
LogTemplateOptions template_options;

LogMessage *
_create_message(void)
{
  LogMessage *msg;
  const gchar *text = "<134>1 2009-10-16T11:51:56+02:00 exchange.macartney.esbjerg MSExchange_ADAccess 20208 _MSGID_ [origin ip=\"exchange.macartney.esbjerg\"][meta sequenceId=\"191732\" sysUpTime=\"68807696\"][EventData@18372.4 Data=\"MSEXCHANGEOWAAPPPOOL.CONFIG\\\" -W \\\"\\\" -M 1 -AP \\\"MSEXCHANGEOWAAPPPOOL5244fileserver.macartney.esbjerg CDG 1 7 7 1 0 1 1 7 1 mail.macartney.esbjerg CDG 1 7 7 1 0 1 1 7 1 maindc.macartney.esbjerg CD- 1 6 6 0 0 1 1 6 1 \"][Keywords@18372.4 Keyword=\"Classic\"] ApplicationMSExchangeADAccess: message";

  msg = log_msg_new(text, strlen(text), NULL, &parse_options);
  log_msg_set_tag_by_name(msg, "almafa");
  return msg;
}

static LogTemplate *
_create_template(const gchar *type_hint_string, const gchar *template_string)
{
  LogTemplate *template;

  template = log_template_new(configuration, NULL);
  log_template_compile(template, template_string, NULL);
  log_template_set_type_hint(template, type_hint_string, NULL);
  return template;
}


/*
 * Tests
 */

struct value_pairs_params {
  const gchar *scope;
  const gchar *exclude;
  const gchar *expected;
  GPtrArray *transformers;
};

void
setup(void)
{
  app_startup();
  putenv("TZ=MET-1METDST");
  tzset();

  configuration = cfg_new(0x0302);
  plugin_load_module("syslogformat", configuration, NULL);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, configuration);
  parse_options.flags |= LP_SYSLOG_PROTOCOL;
}

void
teardown(void)
{
  app_shutdown();
}

TestSuite(value_pairs, .init = setup, .fini = teardown);

ParameterizedTestParameters(value_pairs, test_value_pairs)
{
  static struct value_pairs_params params[] = {
    {"rfc3164", NULL, "DATE,FACILITY,HOST,MESSAGE,PID,PRIORITY,PROGRAM", NULL},
    {"core", NULL, "DATE,FACILITY,HOST,MESSAGE,PID,PRIORITY,PROGRAM", NULL},
    {"base", NULL, "DATE,FACILITY,HOST,MESSAGE,PID,PRIORITY,PROGRAM", NULL},
    {"rfc5424", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip,DATE,FACILITY,HOST,MESSAGE,MSGID,PID,PRIORITY,PROGRAM", NULL},
    {"syslog-proto", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip,DATE,FACILITY,HOST,MESSAGE,MSGID,PID,PRIORITY,PROGRAM", NULL},
    {"selected-macros", NULL, "DATE,FACILITY,HOST,MESSAGE,PID,PRIORITY,PROGRAM,SEQNUM,SOURCEIP,TAGS", NULL},
    {"nv-pairs", NULL, "HOST,MESSAGE,MSGID,PID,PROGRAM", NULL},
    {"dot-nv-pairs", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip", NULL},
    {"sdata", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip", NULL},
    {"all-nv-pairs", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip,HOST,MESSAGE,MSGID,PID,PROGRAM", NULL},
    {"everything", NULL, ".SDATA.EventData@18372.4.Data,.SDATA.Keywords@18372.4.Keyword,.SDATA.meta.sequenceId,.SDATA.meta.sysUpTime,.SDATA.origin.ip,AMPM,BSDTAG,C_DATE,C_DAY,C_FULLDATE,C_HOUR,C_ISODATE,C_MIN,C_MONTH,C_MONTH_ABBREV,C_MONTH_NAME,C_MONTH_WEEK,C_SEC,C_STAMP,C_TZ,C_TZOFFSET,C_UNIXTIME,C_WEEK,C_WEEKDAY,C_WEEK_DAY,C_WEEK_DAY_ABBREV,C_WEEK_DAY_NAME,C_YEAR,C_YEAR_DAY,DATE,DAY,FACILITY,FACILITY_NUM,FULLDATE,HOST,HOSTID,HOUR,HOUR12,ISODATE,LEVEL,LEVEL_NUM,LOGHOST,MESSAGE,MIN,MONTH,MONTH_ABBREV,MONTH_NAME,MONTH_WEEK,MSEC,MSG,MSGHDR,MSGID,PID,PRI,PRIORITY,PROGRAM,R_AMPM,R_DATE,R_DAY,R_FULLDATE,R_HOUR,R_HOUR12,R_ISODATE,R_MIN,R_MONTH,R_MONTH_ABBREV,R_MONTH_NAME,R_MONTH_WEEK,R_MSEC,R_SEC,R_STAMP,R_TZ,R_TZOFFSET,R_UNIXTIME,R_USEC,R_WEEK,R_WEEKDAY,R_WEEK_DAY,R_WEEK_DAY_ABBREV,R_WEEK_DAY_NAME,R_YEAR,R_YEAR_DAY,SDATA,SEC,SEQNUM,SOURCEIP,STAMP,SYSUPTIME,S_AMPM,S_DATE,S_DAY,S_FULLDATE,S_HOUR,S_HOUR12,S_ISODATE,S_MIN,S_MONTH,S_MONTH_ABBREV,S_MONTH_NAME,S_MONTH_WEEK,S_MSEC,S_SEC,S_STAMP,S_TZ,S_TZOFFSET,S_UNIXTIME,S_USEC,S_WEEK,S_WEEKDAY,S_WEEK_DAY,S_WEEK_DAY_ABBREV,S_WEEK_DAY_NAME,S_YEAR,S_YEAR_DAY,TAG,TAGS,TZ,TZOFFSET,UNIXTIME,USEC,WEEK,WEEKDAY,WEEK_DAY,WEEK_DAY_ABBREV,WEEK_DAY_NAME,YEAR,YEAR_DAY", NULL},
    {"nv-pairs", ".SDATA.*", "HOST,MESSAGE,MSGID,PID,PROGRAM", NULL},

    /* tests that the exclude patterns do not affect explicitly added
     * keys. The testcase function adds a TEST_KEY and then checks if
     * it is indeed present. Even if it would be excluded it still has
     * to be in the result set. */
    {"rfc3164", "test.*", "DATE,FACILITY,HOST,MESSAGE,PID,PRIORITY,PROGRAM", NULL},

    /* tests that excluding works even when the key would be in the
     * default set. */
    {"nv-pairs", "MESSAGE", "HOST,MSGID,PID,PROGRAM", NULL}
  };

  return cr_make_param_array(struct value_pairs_params, params, sizeof (params) / sizeof(struct value_pairs_params));
}

ParameterizedTest(struct value_pairs_params *params, value_pairs, test_value_pairs)
{
  ValuePairs *vp;
  GList *vp_keys_list = NULL;
  GString *vp_keys;
  LogMessage *msg = _create_message();
  gpointer args[2];
  gboolean test_key_found = FALSE;
  LogTemplate *template;

  vp_keys = g_string_sized_new(0);

  vp = value_pairs_new();
  value_pairs_add_scope(vp, params->scope);
  if (params->exclude)
    value_pairs_add_glob_pattern(vp, params->exclude, FALSE);
  template = _create_template("string", "$MESSAGE");
  value_pairs_add_pair(vp, TEST_KEY, template);
  log_template_unref(template);

  if (params->transformers)
    {
      gint i;
      ValuePairsTransformSet *vpts = value_pairs_transform_set_new("*");

      for (i = 0; i < params->transformers->len; i++)
        value_pairs_transform_set_add_func(vpts, g_ptr_array_index(params->transformers, i));
      value_pairs_add_transforms(vp, vpts);
    }

  args[0] = &vp_keys_list;
  args[1] = &test_key_found;
  value_pairs_foreach(vp, _vp_keys_foreach, msg, 11, LTZ_LOCAL, &template_options, args);
  g_list_foreach(vp_keys_list, (GFunc) _cat_keys_foreach, vp_keys);

  cr_assert_str_eq(vp_keys->str, params->expected,
    "Scope keys mismatch, scope=[%s], exclude=[%s], value=[%s], expected=[%s]",
    params->scope,
    params->exclude ? params->exclude : "(none)",
    vp_keys->str, params->expected);

  cr_assert(test_key_found, TEST_KEY " is not found in the result set");

  g_list_foreach(vp_keys_list, (GFunc) g_free, NULL);
  g_list_free(vp_keys_list);
  g_string_free(vp_keys, TRUE);
  log_msg_unref(msg);
  value_pairs_unref(vp);
}
