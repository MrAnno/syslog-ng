#include <criterion/criterion.h>
#include "serialize.h"
#include "apphook.h"


TestSuite(serialize, .init = app_startup, .fini = app_shutdown);

Test(serialize, test_serialize)
{
  GString *stream = g_string_new("");
  GString *value = g_string_new("");
  gchar buf[256];
  guint32 num;

  SerializeArchive *a = serialize_string_archive_new(stream);

  serialize_write_blob(a, "MAGIC", 5);
  serialize_write_uint32(a, 0xdeadbeaf);
  serialize_write_cstring(a, "kismacska", -1);
  serialize_write_cstring(a, "tarkabarka", 10);

  serialize_archive_free(a);

  a = serialize_string_archive_new(stream);

  serialize_read_blob(a, buf, 5);
  cr_assert_arr_eq(buf, "MAGIC", 5);

  serialize_read_uint32(a, &num);
  cr_assert_eq(num, 0xdeadbeaf);

  serialize_read_string(a, value);
  cr_assert_str_eq(value->str, "kismacska");

  serialize_read_string(a, value);
  cr_assert_str_eq(value->str, "tarkabarka");
}
