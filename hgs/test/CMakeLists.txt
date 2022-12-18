cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

add_executable(test_Individual test_Individual.cpp)
target_link_libraries(test_Individual GTest::gtest_main)

gtest_discover_tests(test_Individual)
