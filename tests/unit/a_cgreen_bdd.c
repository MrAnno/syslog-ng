#include <cgreen/cgreen.h>

Describe(Strlen);
BeforeEach(Strlen) {}
AfterEach(Strlen) {}

Ensure(Strlen, returns_five_for_hello) {
  assert_that(strlen("Hello"), is_equal_to(5));
}

TestSuite *our_tests() {
  TestSuite *suite = create_test_suite();
  add_test_with_context(suite, Strlen, returns_five_for_hello); 
  return suite;
}

int main(int argc, char **argv) {
  TestSuite *suite = create_test_suite();
  add_suite(suite, our_tests());
  if (argc > 1) {
    return run_single_test(suite, argv[1], create_text_reporter());
  }
  return run_test_suite(suite, create_text_reporter());
}