from numpy.testing import assert_equal
from pytest import mark

from pyvrp._pyvrp import Matrix, TimeWindowSegment


@mark.parametrize("existing_time_warp", [2, 5, 10])
def test_total_time_warp(existing_time_warp):
    tws1 = TimeWindowSegment(0, 0, 0, existing_time_warp, 0, 0, 0)
    assert_equal(tws1.total_time_warp(), existing_time_warp)


def test_merge_two():
    tws1 = TimeWindowSegment(0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(1, 1, 0, 5, 3, 6, 0)

    mat = Matrix([[1, 4], [1, 2]])
    merged = TimeWindowSegment.merge(mat, tws1, tws2)

    # There is no release time, so segment time warp and total time warp should
    # be equal. The first stop (tws1) takes already has five duration, and
    # starts at time 0. Then, we have to drive for 4 units (mat(0, 1) = 4) to
    # get to the second stop (tws2). This second segment has 5 time warp, and
    # we arrive there at time 5 + 4 = 9, which is 9 - 6 = 3 after its closing
    # time window. So we get a final time warp of 5 + 3 = 8.
    assert_equal(merged.total_time_warp(), 8)

    # Now, let's add a bit of release time (3), so that the total time warp
    # should become 8 + 3 = 11.
    tws2 = TimeWindowSegment(1, 1, 0, 5, 3, 6, 3)
    merged = TimeWindowSegment.merge(mat, tws1, tws2)
    assert_equal(merged.total_time_warp(), 11)


def test_merge_multiple():
    tws1 = TimeWindowSegment(0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(1, 1, 0, 0, 3, 6, 0)
    tws3 = TimeWindowSegment(2, 2, 0, 0, 2, 3, 2)

    mat = Matrix([[1, 4, 1], [1, 2, 4], [1, 1, 1]])
    merged1 = TimeWindowSegment.merge(mat, tws1, tws2)
    merged2 = TimeWindowSegment.merge(mat, merged1, tws3)
    merged3 = TimeWindowSegment.merge(mat, tws1, tws2, tws3)

    # Merge all together should be the same as merging in several steps.
    assert_equal(merged3.total_time_warp(), merged2.total_time_warp())

    # After also merging in tws3, we should have 3 time warp from 0 -> 1, and 7
    # time warp from 1 -> 2. Since there's also a release time of 2, the total
    # time warp is 12.
    assert_equal(merged3.total_time_warp(), 12)


# TODO test two previously merged segments, e.g. merge(merge(1, 2),
#  merge(3, 4)), each with time warp
