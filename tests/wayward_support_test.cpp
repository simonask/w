#include <gtest/gtest.h>
#include "wayward/support/cloning_ptr_test.cpp"
#include "wayward/support/format_test.cpp"
#include "wayward/support/json_test.cpp"

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
