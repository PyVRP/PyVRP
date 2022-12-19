cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

add_executable(test_CommandLine test_CommandLine.cpp)
target_link_libraries(test_CommandLine GTest::gtest_main)
gtest_discover_tests(test_CommandLine)

add_executable(test_Matrix test_Matrix.cpp)
target_link_libraries(test_Matrix GTest::gtest_main)
gtest_discover_tests(test_Matrix)

add_executable(test_TimeWindowSegment test_TimeWindowSegment.cpp)
target_link_libraries(test_TimeWindowSegment GTest::gtest_main)
gtest_discover_tests(test_TimeWindowSegment)

add_executable(test_XorShift128 test_XorShift128.cpp)
target_link_libraries(test_XorShift128 GTest::gtest_main)
gtest_discover_tests(test_XorShift128)
