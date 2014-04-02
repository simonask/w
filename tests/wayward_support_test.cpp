#include <gtest/gtest.h>

TEST(TestCase, TestCaseName) {
  EXPECT_EQ(2, 1+1);
}

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
