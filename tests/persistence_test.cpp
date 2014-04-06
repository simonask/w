#include <gtest/gtest.h>
#include "persistence/relational_algebra_test.cpp"

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
