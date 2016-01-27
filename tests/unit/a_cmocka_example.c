#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h> // include before cmocka.h - but why?!
#include <cmocka.h>
#include <stdlib.h>

/*
 * https://api.cmocka.org/group__cmocka__asserts.html
 */

static int
setup(void **state)
{
  *state = malloc(sizeof(int));

  int *random_value = *state;
  *random_value = 1;

  return 0;
}

static int
teardown(void **state)
{
  free(*state);
  return 0;
}


static void
test_one(void **state)
{
  int *random_value = *state;

  assert_true(*random_value);
}

static void
test_two(void **state)
{
  (void) state; /* unused */

  assert_false(1);

  /*
    assert_false (scalar expression)
    assert_true (scalar expression)
    assert_in_range (LargestIntegralType value, LargestIntegralType minimum, LargestIntegralType maximum)
    assert_in_set (LargestIntegralType value, LargestIntegralType values[], size_t count)
    assert_int_equal (int a, int b)
    assert_int_not_equal (int a, int b)
    assert_memory_equal (const void *a, const void *b, size_t size)
    assert_memory_not_equal (const void *a, const void *b, size_t size)
    assert_non_null (void *pointer)
    assert_not_in_range (LargestIntegralType value, LargestIntegralType minimum, LargestIntegralType maximum)
    assert_not_in_set (LargestIntegralType value, LargestIntegralType values[], size_t count)
    assert_null (void *pointer)
    assert_ptr_equal (void *a, void *b)
    assert_ptr_not_equal (void *a, void *b)
    assert_return_code (int rc, int error)
    assert_string_equal (const char *a, const char *b)
    assert_string_not_equal (const char *a, const char *b)
  */
}

int
main(void)
{
  struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_one, setup, teardown),
    cmocka_unit_test(test_two)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
