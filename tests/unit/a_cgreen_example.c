#include <cgreen/cgreen.h>

/*
 * https://cgreen-devs.github.io/#_the_standard_constraints
 */

Describe(SyslogNG);
BeforeEach(SyslogNG) {}
AfterEach(SyslogNG) {}

Ensure(SyslogNG, passes_this_test)
{
  assert_that(1 == 1);
}

Ensure(SyslogNG, fails_this_test)
{
  assert_that(0 == 1);
}

// auto discovery with 'cgreen-runner'
int main(int argc, char **argv)
{
  TestSuite *suite = create_test_suite();
  add_test_with_context(suite, SyslogNG, passes_this_test);
  add_test_with_context(suite, SyslogNG, fails_this_test);
  return run_test_suite(suite, create_text_reporter());
}
