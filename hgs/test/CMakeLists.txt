cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

add_subdirectory(diversity)
add_subdirectory(stop)

# Individual tests have file dependencies that require fixing the working directory
add_executable(test_Individual test_Individual.cpp)
target_link_libraries(test_Individual GTest::gtest_main individual)
gtest_discover_tests(test_Individual WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test)

add_executable(test_Matrix test_Matrix.cpp)
target_link_libraries(test_Matrix GTest::gtest_main)
gtest_discover_tests(test_Matrix)

# ProblemData tests have file dependencies that require fixing the working directory
add_executable(test_ProblemData test_ProblemData.cpp)
target_link_libraries(test_ProblemData GTest::gtest_main problemData)
gtest_discover_tests(test_ProblemData WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test)

add_executable(test_PenaltyManager test_PenaltyManager.cpp)
target_link_libraries(test_PenaltyManager GTest::gtest_main penaltyManager)
gtest_discover_tests(test_PenaltyManager)

# Population tests have file dependencies that require fixing the working directory
add_executable(test_Population test_Population.cpp)
target_link_libraries(test_Population GTest::gtest_main population diversityMeasures)
gtest_discover_tests(test_Population WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test)

add_executable(test_TimeWindowSegment test_TimeWindowSegment.cpp)
target_link_libraries(test_TimeWindowSegment GTest::gtest_main)
gtest_discover_tests(test_TimeWindowSegment)

add_executable(test_XorShift128 test_XorShift128.cpp)
target_link_libraries(test_XorShift128 GTest::gtest_main xorShift128)
gtest_discover_tests(test_XorShift128)
