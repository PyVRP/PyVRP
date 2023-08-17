import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp._pyvrp import TimeWindowSegment


@mark.parametrize("existing_time_warp", [2, 5, 10])
def test_total_time_warp_when_there_is_existing_time_warp(existing_time_warp):
    """
    Tests that the ``total_time_warp()`` returns existing time warp when no
    segments have been merged yet.
    """
    tws1 = TimeWindowSegment(0, 0, 0, existing_time_warp, 0, 0, 0)
    assert_equal(tws1.total_time_warp(), existing_time_warp)


def test_merge_two():
    """
    Tests merging two time window segments.
    """
    tws1 = TimeWindowSegment(0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(1, 1, 0, 5, 3, 6, 0)

    mat = np.asarray([[1, 4], [1, 2]])
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


def test_merge_three():
    """
    Tests merging three time window segments.
    """
    tws1 = TimeWindowSegment(0, 0, 5, 0, 0, 5, 0)
    tws2 = TimeWindowSegment(1, 1, 0, 0, 3, 6, 0)
    tws3 = TimeWindowSegment(2, 2, 0, 0, 2, 3, 2)

    mat = np.asarray([[1, 4, 1], [1, 2, 4], [1, 1, 1]])
    merged1 = TimeWindowSegment.merge(mat, tws1, tws2)
    merged2 = TimeWindowSegment.merge(mat, merged1, tws3)
    merged3 = TimeWindowSegment.merge(mat, tws1, tws2, tws3)

    # Merge all together should be the same as merging in several steps.
    assert_equal(merged3.total_time_warp(), merged2.total_time_warp())

    # After also merging in tws3, we should have 3 time warp from 0 -> 1, and 7
    # time warp from 1 -> 2. Since there's also a release time of 2, the total
    # time warp is 12.
    assert_equal(merged3.total_time_warp(), 12)


def test_merging_two_previously_merged_tws():
    """
    This test evaluates what happens when we merge two previously merged
    segments, when both have time warp.
    """
    time_warp = 1
    tws1 = TimeWindowSegment(0, 0, 5, time_warp, 0, 5, 0)  # depot
    tws2 = TimeWindowSegment(1, 1, 1, time_warp, 3, 6, 0)  # client #1

    # Each of these segments has some initial time warp.
    assert_equal(tws1.total_time_warp(), 1)
    assert_equal(tws2.total_time_warp(), 1)

    mat = np.asarray([[0, 4], [3, 0]])
    merged12 = TimeWindowSegment.merge(mat, tws1, tws2)  # depot -> client
    merged21 = TimeWindowSegment.merge(mat, tws2, tws1)  # client -> depot

    # The order in which time window segments are merged matters, so although
    # these two segments cover the same clients, the total time warp is not
    # the same.
    assert_(merged12.total_time_warp() != merged21.total_time_warp())

    # Both segments start with 1 initial time warp. Going from depot -> client
    # happens at t = 5 - 1, and takes 4 time units to complete. So we arrive at
    # t = 8, which is three units after the time window closes. This adds 2
    # time warp to the segment, which, together with the initial time warps
    # makes for 2 + 1 + 1 = 4 total time warp.
    assert_equal(merged12.total_time_warp(), 4)

    # We can leave at the earliest at t = 3, the start of the client's time
    # window. We then arrive at t = 6, which adds one unit of time warp.
    # Combined with the initial time warp, this results in 1 + 1 + 1 = 3 units
    # of total time warp.
    assert_equal(merged21.total_time_warp(), 3)

    # This merged TWS represents the route plan 0 -> 1 -> 1 -> 0. We leave 1
    # at t = 10 - 4, and travel takes no time. We do get the 1 unit of
    # existing time warp. We then travel to the depot, arriving at t = 9. This
    # is 4 time units after the time window closes, which adds 4 time warp
    # (plus the existing unit). So we get 4 + 1 + 1 + 4 = 10 time warp.
    merged = TimeWindowSegment.merge(mat, merged12, merged21)
    assert_equal(merged.total_time_warp(), 10)
