from numpy.testing import assert_equal

from pyvrp._lib.hgspy import IntMatrix, TimeWindowSegment


def test_segment_time_warp():
    mat = IntMatrix(0)

    # Zero existing time warp, so expect 0 time warp on segment.
    tws1 = TimeWindowSegment(mat, 0, 0, 0, 0, 0, 0, 0)
    assert_equal(tws1.segment_time_warp(), 0)

    # 5 existing time warp, so expect 5 segment time warp.
    tws2 = TimeWindowSegment(mat, 0, 0, 0, 5, 0, 0, 0)
    assert_equal(tws2.segment_time_warp(), 5)


def test_total_time_warp():
    mat = IntMatrix(0)

    # 5 time warp passed in, so we expect 5 time warp.
    tws1 = TimeWindowSegment(mat, 0, 0, 0, 5, 0, 0, 0)
    assert_equal(tws1.total_time_warp(), 5)

    # 5 time warp passed in, together with 0 twLate and 5 release: we expect
    # 5 segment time warp, and 10 total time warp (due to release time).
    tws2 = TimeWindowSegment(mat, 0, 0, 0, 5, 0, 0, 5)
    assert_equal(tws2.segment_time_warp(), 5)
    assert_equal(tws2.total_time_warp(), 10)


def test_merge_two():
    mat = IntMatrix([[1, 4], [1, 2]])
    tws1 = TimeWindowSegment(mat, 0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(mat, 1, 1, 0, 5, 3, 6, 0)

    merged = TimeWindowSegment.merge(tws1, tws2)

    # There is no release time, so segment time warp and total time warp should
    # be equal. The first stop (tws1) takes already has five duration, and
    # starts at time 0. Then, we have to drive for 4 units (mat(0, 1) = 4) to
    # get to the second stop (tws2). This second segment has 5 time warp, and
    # we arrive there at time 5 + 4 = 9, which is 9 - 6 = 3 after its closing
    # time window. So we get a final time warp of 5 + 3 = 8.
    assert_equal(merged.segment_time_warp(), 8)
    assert_equal(merged.total_time_warp(), 8)

    # Now, let's add a bit of release time (3) to the computation.
    tws2 = TimeWindowSegment(mat, 1, 1, 0, 5, 3, 6, 3)
    merged = TimeWindowSegment.merge(tws1, tws2)

    # Nothing has changed to the segment time warp, but the total time warp
    # should now include the release time (3), so 8 + 3 = 11.
    assert_equal(merged.segment_time_warp(), 8)
    assert_equal(merged.total_time_warp(), 11)


def test_merge_multiple():
    mat = IntMatrix([[1, 4, 1], [1, 2, 4], [1, 1, 1]])
    tws1 = TimeWindowSegment(mat, 0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(mat, 1, 1, 0, 0, 3, 6, 0)
    tws3 = TimeWindowSegment(mat, 2, 2, 0, 0, 2, 3, 2)

    merged1 = TimeWindowSegment.merge(tws1, tws2)
    merged2 = TimeWindowSegment.merge(merged1, tws3)
    merged3 = TimeWindowSegment.merge(tws1, tws2, tws3)

    # Merge all together should be the same as merging in several steps.
    assert_equal(merged3.segment_time_warp(), merged3.segment_time_warp())
    assert_equal(merged3.total_time_warp(), merged2.total_time_warp())

    # After also merging in tws3, we should have 3 time warp from 0 -> 1, and 7
    # time warp from 1 -> 2, for 10 segment time warp. Since there's also a
    # release time of 2, the total time warp is 12.
    assert_equal(merged3.segment_time_warp(), 10)
    assert_equal(merged3.total_time_warp(), 12)


# TODO test two previously merged segments, e.g. merge(merge(1, 2),
#  merge(3, 4)), each with time warp
