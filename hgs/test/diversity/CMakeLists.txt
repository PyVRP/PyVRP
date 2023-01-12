cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

# brokenPairsDistance tests have file dependencies that require fixing the working directory
add_executable(test_brokenPairsDistance test_brokenPairsDistance.cpp)
target_link_libraries(test_brokenPairsDistance GTest::gtest_main diversityMeasures penaltyManager)
gtest_discover_tests(test_brokenPairsDistance WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test)
