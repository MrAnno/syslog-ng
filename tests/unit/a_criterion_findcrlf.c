#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include "find-crlf.h"

struct findcrlf_params {
  gchar *msg;
  gsize msg_len;
  gsize eom_ofs;
};

ParameterizedTestParameters(findcrlf, test) {
  static struct findcrlf_params params[] = {
    {"a\nb\nc\n",  6,  1},
    {"ab\nb\nc\n",  7,  2},
    {"abc\nb\nc\n",  8,  3},
    {"abcd\nb\nc\n",  9,  4},
    {"abcde\nb\nc\n", 10,  5},
    {"abcdefghijklmnopqrstu\nb\nc\n", 26, 21},
    {"abcdefghijklmnopqrstuv\nb\nc\n", 27, 22},
    {"abcdefghijklmnopqrstuvw\nb\nc\n", 28, 23},
    {"abcdefghijklmnopqrstuvwx\nb\nc\n", 29, 24},
    {"abcdefghijklmnopqrstuvwxy\nb\nc\n", 30, 25},
    {"abcdefghijklmnopqrstuvwxyz\nb\nc\n", 31, 26},

    {"a\rb\rc\r",  6,  1},
    {"ab\rb\rc\r",  7,  2},
    {"abc\rb\rc\r",  8,  3},
    {"abcd\rb\rc\r",  9,  4},
    {"abcde\rb\rc\r", 10,  5},
    {"abcdefghijklmnopqrst\rb\rc\r", 25, 20},
    {"abcdefghijklmnopqrstu\rb\rc\r", 26, 21},
    {"abcdefghijklmnopqrstuv\rb\rc\r", 27, 22},
    {"abcdefghijklmnopqrstuvw\rb\rc\r", 28, 23},
    {"abcdefghijklmnopqrstuvwx\rb\rc\r", 29, 24},
    {"abcdefghijklmnopqrstuvwxy\rb\rc\r", 30, 25},
    {"abcdefghijklmnopqrstuvwxyz\rb\rc\r", 31, 26},

    {"a",  1, -1},
    {"ab",  2, -1},
    {"abc",  3, -1},
    {"abcd",  4, -1},
    {"abcde",  5, -1},
    {"abcdef",  6, -1},
    {"abcdefg",  7, -1},
    {"abcdefgh",  8, -1},
    {"abcdefghi",  9, -1},
    {"abcdefghij", 10, -1},
    {"abcdefghijklmnopqrst", 20, -1},
    {"abcdefghijklmnopqrstu", 21, -1},
    {"abcdefghijklmnopqrstuv", 22, -1},
    {"abcdefghijklmnopqrstuvw", 23, -1},
    {"abcdefghijklmnopqrstuvwx", 24, -1},
    {"abcdefghijklmnopqrstuvwxy", 25, -1},
    {"abcdefghijklmnopqrstuvwxyz", 26, -1},
  };

  return cr_make_param_array(struct findcrlf_params, params, sizeof (params) / sizeof(struct findcrlf_params));
}

ParameterizedTest(struct findcrlf_params *params, findcrlf, test)
{
  gchar *eom;

  eom = find_cr_or_lf(params->msg, params->msg_len);

  cr_expect_not(params->eom_ofs == -1 && eom != NULL,
                "EOM returned is not NULL, which was expected. eom_ofs=%d, eom=%s\n",
                (gint) params->eom_ofs, eom);

  if (params->eom_ofs == -1)
    return;

  cr_expect_not(eom - params->msg != params->eom_ofs,
                "EOM is at wrong location. msg=%s, eom_ofs=%d, eom=%s\n",
                params->msg, (gint) params->eom_ofs, eom);
}
