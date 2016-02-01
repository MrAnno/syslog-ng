#include <unity.h>

/*
 * https://github.com/ThrowTheSwitch/Unity/blob/master/docs/Unity%20Summary.pdf
 * We need to run a ruby generator script (at the moment, it's in the autogen.sh).
 */

int Counter;

void setUp(void)
{
  //This is run before EACH TEST
  Counter = 0x5a5a;
}

void tearDown(void)
{
}

void test_1(void)
{
  TEST_ASSERT_EQUAL(0, 0);
}

void test_2(void)
{
  TEST_ASSERT_TRUE(1);
  /*
   * TEST_ASSERT_FALSE(condition)
   * TEST_ASSERT(condition)
   * TEST_FAIL()
   * TEST_FAIL_MESSAGE(message)
   * TEST_ASSERT_EQUAL(expected, actual)
   * TEST_ASSERT_EQUAL_INT(expected, actual)
   * TEST_ASSERT_INT_WITHIN(delta, expected, actual)
   * TEST_ASSERT_EQUAL_STRING(expected, actual)
   * ...
   */
}
