#include <criterion/criterion.h>

#include "value-pairs/value-pairs.h"
#include "apphook.h"
#include "plugin.h"
#include "cfg.h"
#include "logmsg/logmsg.h"

#include <string.h>

MsgFormatOptions parse_options;
LogTemplateOptions template_options;

gint root_data = 1;
gint root_test_data = 2;

static gboolean
assert_vp_obj_start(const gchar *name,
                       const gchar *prefix, gpointer *prefix_data,
                       const gchar *prev, gpointer *prev_data,
                       gpointer user_data)
{
  static int times_called = 0;

  switch(times_called)
    {
      case 0:
         cr_assert_eq(prefix, 0, "First vp_obj_start but prefix is not NULL!");
         break;
      case 1:
         cr_assert_str_eq(prefix, "root", "Second vp_obj_start but prefix is not 'root'!");
         *prefix_data = &root_data;
         break;
      case 2:
         cr_assert_str_eq(prefix, "root.test", "Third vp_obj_start but prefix is not 'root.test'!");
         cr_assert_str_eq(prev, "root", "Wrong previous prefix");
         cr_assert_eq(*((gint*)(*prev_data)), root_data, "Wrong previous data");
         *prefix_data = &root_test_data;
         break;
      default:
         cr_assert(FALSE, "vp_obj_start called more times than number of path elements!");
    }
  times_called++;
  return FALSE;
}

static gboolean
assert_vp_obj_stop(const gchar *name,
                       const gchar *prefix, gpointer *prefix_data,
                       const gchar *prev, gpointer *prev_data,
                       gpointer user_data)
{
  static int times_called = 0;

  switch(times_called)
    {
      case 0:
         cr_assert_str_eq(prefix, "root.test", "First vp_obj_stop but prefix is not 'root.test'!");
         cr_assert_str_eq(prev, "root", "Wrong previous prefix");
         cr_assert_eq(*((gint*)(*prev_data)), root_data, "Wrong previous data");
         break;
      case 1:
         cr_assert_str_eq(prefix, "root", "Second vp_obj_stop but prefix is not 'root'!");
         break;
      case 2:
         cr_assert_eq(prefix, 0, "Third vp_obj_stop but prefix is not NULL!");
         break;
      default:
         cr_assert(FALSE, "vp_obj_stop called more times than number of path elements!");
    }
  times_called++;
  return FALSE;

}

static gboolean
assert_vp_value(const gchar *name, const gchar *prefix,
                           TypeHint type, const gchar *value, gsize value_len,
                           gpointer *prefix_data, gpointer user_data)
{
  cr_assert_str_eq(prefix, "root.test", "Wrong prefix");
  cr_assert_eq(value_len, strlen("value"), "Wrong value");
  cr_assert_arr_eq(value, "value", value_len, "Wrong value");
  cr_assert_eq(*((gint*)(*prefix_data)), root_test_data, "Wrong prefix data");

  return FALSE;
}


/*
 * Tests
 */

void
setup(void)
{
  app_startup();

  configuration = cfg_new(0x0303);
  plugin_load_module("syslogformat", configuration, NULL);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, configuration);
}

void
teardown(void)
{
  app_shutdown();
}

TestSuite(value_pairs_walk, .init = setup, .fini = teardown);

Test(value_pairs_walk, test_value_pairs_walk_prefix_data)
{
  ValuePairs *vp;
  LogMessage *msg;
  const char* value = "value";

  log_template_options_init(&template_options, configuration);
  msg_format_options_init(&parse_options, configuration);

  vp = value_pairs_new();
  value_pairs_add_glob_pattern(vp, "root.*", TRUE);
  msg = log_msg_new("test", 4, NULL, &parse_options);

  log_msg_set_value_by_name(msg, "root.test.alma", value, strlen(value));
  log_msg_set_value_by_name(msg, "root.test.korte", value, strlen(value));

  value_pairs_walk(vp, assert_vp_obj_start, assert_vp_value, assert_vp_obj_stop, msg, 0, LTZ_LOCAL, &template_options, NULL);
  value_pairs_unref(vp);
  log_msg_unref(msg);
}
