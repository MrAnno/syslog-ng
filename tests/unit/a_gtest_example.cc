#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmock/cmock.h>

/*
 * https://github.com/google/googletest/blob/master/googletest/docs/Primer.md
 * https://github.com/google/googletest/blob/master/googletest/docs/Samples.md
 * https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md
 */

namespace {

// Fixture-class (optional)
class SyslogNGTest : public ::testing::Test {
protected:
  SyslogNGTest()
  {
    // You can do set-up work for each test here.
  }

  virtual ~SyslogNGTest()
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  virtual void SetUp()
  {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown()
  {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for SyslogNGTest.
};

// Test with fixture
TEST_F(SyslogNGTest, Test1)
{
  EXPECT_FALSE(true) << "Optional error description.";

  ASSERT_EQ(123, 123);
  EXPECT_EQ(123, 321);

  /*
    ASSERT_TRUE(condition);    EXPECT_TRUE(condition);
    ASSERT_FALSE(condition);   EXPECT_FALSE(condition);

    EXPECT_EQ(expected,actual);
    EXPECT_NE(val1,val2);
    EXPECT_LT(val1,val2);
    EXPECT_LE(val1,val2);
    EXPECT_GT(val1,val2);
    EXPECT_GE(val1,val2);

    EXPECT_STREQ(expected_str,actual_str);
    EXPECT_STRNE(str1,str2);
    EXPECT_STRCASEEQ(expected_str,actual_str); (ignoring case)
    EXPECT_STRCASENE(str1,str2); (ignoring case)
  */
}

// TEST(test_case_name, test_name)
// Test with fixture
TEST(TestWithoutFixture, Test2)
{
  ASSERT_STREQ("fatal", "faatl");
  EXPECT_STRNE("nonfatal", "nonfatal");
}


}  // namespace
