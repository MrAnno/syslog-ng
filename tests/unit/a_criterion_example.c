#include <criterion/criterion.h>
#include <stdio.h>
#include <signal.h>

/*
 * http://criterion.readthedocs.org/en/latest/assert.html
 */

void
setup_suite(void)
{
  puts("Runs before the tests");
}

void
teardown_suite(void)
{
  puts("Runs after the tests");
}

TestSuite(first_suite, .init = setup_suite, .fini = teardown_suite /*, .disabled = true */);

Test(first_suite, test_name)
{
  cr_assert(true, "Assertions may take failure messages");
  cr_assert(true, "Or even %d format string %s", 1, "with parameters");
  cr_expect(false, "assert is fatal, expect isn't");

  cr_assert_str_not_empty("foo");
  cr_assert_str_eq("hello", "hello");
}

Test(first_suite, array_test, .description = "Just an array test")
{
  int arr1[] = {1, 2, 3, 4};
  int arr2[] = {4, 3, 2, 1};

  cr_assert_arr_eq(arr1, arr1, 4);
  cr_assert_arr_neq(arr1, arr2, 4);
}

Test(second_suite, caught, .signal = SIGSEGV)
{
  raise(SIGSEGV);
}
