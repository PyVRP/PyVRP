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

TEST(TestTimeWindowSegment, mergeTwo)
{
    Matrix<int> mat({{1, 4}, {1, 2}});
    TimeWindowSegment tws1 = {&mat, 0, 0, 5, 0, 0, 5, 0};
    TimeWindowSegment tws2 = {&mat, 1, 1, 0, 5, 3, 6, 0};

    auto merged = TimeWindowSegment::merge(tws1, tws2);

    // There is no release time, so segment time warp and total time warp
    // should be equal. The first stop (tws1) takes already has five duration,
    // and starts at time 0. Then, we have to drive for 4 units (mat(0, 1) = 4)
    // to get to the second stop (tws2). This second segment has 5 time warp,
    // and we arrive there at time 5 + 4 = 9, which is 9 - 6 = 3 after its
    // closing time window. So we get a final time warp of 5 + 3 = 8.
    ASSERT_EQ(merged.segmentTimeWarp(), 8);
    ASSERT_EQ(merged.totalTimeWarp(), 8);

    // Now, let's add a bit of release time (3) to the computation.
    tws2 = {&mat, 1, 1, 0, 5, 3, 6, 3};
    merged = TimeWindowSegment::merge(tws1, tws2);

    // Nothing has changed to the segment time warp, but the total time warp
    // should now include the release time (3), so 8 + 3 = 11.
    ASSERT_EQ(merged.segmentTimeWarp(), 8);
    ASSERT_EQ(merged.totalTimeWarp(), 11);
}

TEST(TestTimeWindowSegment, mergeMultiple)
{
    Matrix<int> mat({{1, 4, 1}, {1, 2, 4}, {1, 1, 1}});
    TimeWindowSegment tws1 = {&mat, 0, 0, 5, 0, 0, 5, 0};
    TimeWindowSegment tws2 = {&mat, 1, 1, 0, 0, 3, 6, 0};
    TimeWindowSegment tws3 = {&mat, 2, 2, 0, 0, 2, 3, 2};

    auto merged1 = TimeWindowSegment::merge(tws1, tws2);
    auto merged2 = TimeWindowSegment::merge(merged1, tws3);
    auto merged3 = TimeWindowSegment::merge(tws1, tws2, tws3);

    // Merge all together should be the same as merging in several steps.
    ASSERT_EQ(merged3.segmentTimeWarp(), merged2.segmentTimeWarp());
    ASSERT_EQ(merged3.totalTimeWarp(), merged2.totalTimeWarp());

    // After also merging in tws3, we should have 3 time warp from 0 -> 1, and
    // 7 time warp from 1 -> 2, for 10 segment time warp. Since there's also
    // a release time of 2, the total time warp is 12.
    ASSERT_EQ(merged3.segmentTimeWarp(), 10);
    ASSERT_EQ(merged3.totalTimeWarp(), 12);
}

// TODO test two previously merged segments, e.g.
//  merge(merge(1, 2), merge(3, 4)), each with time warp
