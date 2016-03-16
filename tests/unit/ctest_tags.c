#include <criterion/criterion.h>

#include "apphook.h"
#include "tags.h"
#include "logmsg/logmsg.h"
#include "messages.h"
#include "filter/filter-tags.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define NUM_TAGS 8159
#define FILTER_TAGS 100

gchar *
get_tag_by_id(LogTagId id)
{
  return g_strdup_printf("tags%d", id);
}

void
setup(void)
{
  app_startup();
  msg_init(TRUE);
}

void
teardown(void)
{
  app_shutdown();
}

TestSuite(tags, .init = setup, .fini = teardown);

Test(tags, test_tags)
{
  guint i, check;
  guint id;
  gchar *name;
  const gchar *tag_name;

  for (check = 0; check < 2; check++)
    for (i = 0; i < NUM_TAGS; i++)
      {
        name = get_tag_by_id(i);
        id = log_tags_get_by_name(name);

        cr_assert_eq(id, i,
          "%s tag: invalid tag id %d %s", check ? "Checking" : "Adding", id, name);

        g_free(name);
      }

  for (i = 0; i < NUM_TAGS; i++)
    {
      name = get_tag_by_id(i);

      tag_name = log_tags_get_by_id(i);

      cr_assert_not_null(tag_name, "Error looking up tag by id %d %s", i, name);
      cr_assert_str_eq(tag_name, name,
        "Bad tag name for id %d %s (%s)", i, tag_name, name);

      g_free(name);
    }

  for (i = NUM_TAGS; i < (NUM_TAGS + 3); i++)
    {
      tag_name = log_tags_get_by_id(i);
      cr_assert_null(tag_name,
        "Found tag name for invalid id %d %s", i, tag_name);
    }
}

Test(tags, test_msg_tags)
{
  gchar *name;
  gint i, set;

  LogMessage *msg = log_msg_new_empty();
  for (set = 1; set != -1; set--)
    {
      for (i = NUM_TAGS; i > -1; i--)
        {
          name = get_tag_by_id(i);

          if (set)
            log_msg_set_tag_by_name(msg, name);
          else
            log_msg_clear_tag_by_name(msg, name);

          cr_assert_not(set ^ log_msg_is_tag_by_id(msg, i),
            "Tag is %sset now (by id) %d", set ? "not " : "", i);

          g_free(name);
        }
    }
  log_msg_unref(msg);

  msg = log_msg_new_empty();
  for (set = 1; set != -1; set--)
    {
      for (i = 0; i < NUM_TAGS; i++)
        {
          name = get_tag_by_id(i);

          if (set)
            log_msg_set_tag_by_name(msg, name);
          else
            log_msg_clear_tag_by_name(msg, name);

          cr_assert_not(set ^ log_msg_is_tag_by_id(msg, i),
            "Tag is %sset now (by id) %d", set ? "not " : "", i);

          cr_assert_not(set && i < sizeof(gulong) * 8 && msg->num_tags != 0,
            "Small IDs are set which should be stored in-line but num_tags is non-zero");

          g_free(name);
        }
    }
  log_msg_unref(msg);

}

void
test_filters(gboolean not)
{
  LogMessage *msg = log_msg_new_empty();
  FilterExprNode *f = filter_tags_new(NULL);
  guint i;
  GList *l = NULL;

  for (i = 1; i < FILTER_TAGS; i += 3)
    l = g_list_prepend(l, get_tag_by_id(i));

  filter_tags_add(f, l);
  f->comp = not;

  for (i = 0; i < FILTER_TAGS; i++)
    {
      log_msg_set_tag_by_id(msg, i);

      cr_assert_not(((i % 3 == 1) ^ filter_expr_eval(f, msg)) ^ not,
        "Failed to match message by tag %d", i);

      log_msg_clear_tag_by_id(msg, i);

      cr_assert_not(filter_expr_eval(f, msg) ^ not,
        "Failed to match message with no tags");
    }

  filter_expr_unref(f);
  log_msg_unref(msg);
}

Test(tags, test_filters)
{
  test_filters(FALSE);
}

Test(tags, test_filters_not)
{
  test_filters(TRUE);
}
