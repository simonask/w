cmake_minimum_required(VERSION 2.6)

add_subdirectory(googletest)
enable_testing()

set(TESTS wayward_support_test persistence_test wayward_test)

set(TEST_BASE_SOURCES test.cpp)

set(wayward_support_test_CASES
  wayward/support/cloning_ptr_test
  wayward/support/format_test
  wayward/support/json_test
  wayward/support/datetime_test
)
set(wayward_support_test_LINK wayward_support)

set(persistence_test_CASES
  persistence/relational_algebra_test
  persistence/projection_test
)
set(persistence_test_LINK persistence)

add_library(test_base STATIC ${TEST_BASE_SOURCES})
target_include_directories(test_base PRIVATE googletest/include)

foreach(TEST_CASE ${wayward_support_test_CASES})
  add_executable(${TEST_CASE} ${TEST_CASE}.cpp)
  target_include_directories(${TEST_CASE} PRIVATE googletest/include)
  target_link_libraries(${TEST_CASE} gtest wayward_support test_base)
  add_test(NAME ${TEST_CASE} COMMAND ${TEST_CASE})
endforeach(TEST_CASE)

foreach(TEST_CASE ${persistence_test_CASES})
  add_executable(${TEST_CASE} ${TEST_CASE}.cpp)
  target_include_directories(${TEST_CASE} PRIVATE googletest/include)
  target_link_libraries(${TEST_CASE} gtest persistence test_base)
  add_test(NAME ${TEST_CASE} COMMAND ${TEST_CASE})
endforeach(TEST_CASE)
