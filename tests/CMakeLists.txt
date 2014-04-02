cmake_minimum_required(VERSION 2.6)

add_subdirectory(googletest)
enable_testing()

set(TESTS wayward_support_test persistence_test wayward_test)

foreach(TEST ${TESTS})
  add_executable(${TEST} ${TEST}.cpp)
  target_include_directories(${TEST} PRIVATE googletest/include)
  target_link_libraries(${TEST} gtest)
  add_test(NAME ${TEST} COMMAND ${TEST})
endforeach(TEST)

target_link_libraries(persistence_test persistence)
target_link_libraries(wayward_support_test wayward_support)
target_link_libraries(wayward_test wayward)
