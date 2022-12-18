#include <gtest/gtest.h>

#include "Matrix.h"
#include "TimeWindowSegment.h"

TEST(TestTimeWindowSegment, segmentTimeWarp)
{
    Matrix<int> mat(0);

    // 0 time warp passed in, so we expect 0 time warp.
    TimeWindowSegment tws1 = {&mat, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(tws1.segmentTimeWarp(), 0);

    // 5 time warp passed in, so we expect 5 time warp.
    TimeWindowSegment tws2 = {&mat, 0, 0, 0, 5, 0, 0, 0};
    ASSERT_EQ(tws2.segmentTimeWarp(), 5);
}

TEST(TestTimeWindowSegment, totalTimeWarp)
{
    Matrix<int> mat(0);

    // 5 time warp passed in, so we expect 5 time warp.
    TimeWindowSegment tws1 = {&mat, 0, 0, 0, 5, 0, 0, 0};
    ASSERT_EQ(tws1.totalTimeWarp(), 5);

    // 5 time warp passed in, together with 0 twLate and 5 release: we expect
    // 5 segment time warp, and 10 total time warp (due to release time).
    TimeWindowSegment tws2 = {&mat, 0, 0, 0, 5, 0, 0, 5};
    ASSERT_EQ(tws2.totalTimeWarp(), 10);
}

// TODO test merge
