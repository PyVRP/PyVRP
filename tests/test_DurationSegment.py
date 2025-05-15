from itertools import pairwise

import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp._pyvrp import DurationSegment

_INT_MAX = np.iinfo(np.int64).max


@pytest.mark.parametrize("existing_time_warp", [2, 5, 10])
def test_time_warp_when_there_is_existing_time_warp(existing_time_warp):
    """
    Tests that the ``time_warp()`` returns existing time warp when no
    segments have been merged yet.
    """
    ds1 = DurationSegment(0, existing_time_warp, 0, 0, 0)
    assert_equal(ds1.time_warp(), existing_time_warp)


def test_merge_two():
    """
    Tests merging two duration segments.
    """
    ds1 = DurationSegment(5, 0, 0, 5, 0)
    ds2 = DurationSegment(0, 5, 3, 6, 0)

    mat = np.asarray([[1, 4], [1, 2]])
    merged = DurationSegment.merge(mat[0, 1], ds1, ds2)

    # There is no release time. The first stop (ds1) takes already has five
    # duration, and starts at time 0. Then, we have to drive for 4 units
    # (mat(0, 1) = 4) to get to the second stop (ds2). This second segment has
    # 5 time warp, and we arrive there at time 5 + 4 = 9, which is 9 - 6 = 3
    # after its closing time window. So we get a final time warp of 5 + 3 = 8.
    assert_equal(merged.time_warp(), 8)

    # Now, let's add a bit of release time (3), so that the total time warp
    # should become 8 + 3 = 11.
    ds2 = DurationSegment(0, 5, 3, 6, 3)
    merged = DurationSegment.merge(mat[0, 1], ds1, ds2)
    assert_equal(merged.time_warp(), 11)


def test_merging_two_previously_merged_duration_segments():
    """
    This test evaluates what happens when we merge two previously merged
    segments, when both have time warp.
    """
    time_warp = 1
    ds1 = DurationSegment(5, time_warp, 0, 5, 0)  # depot
    ds2 = DurationSegment(1, time_warp, 3, 6, 0)  # client 1

    # Each of these segments has some initial time warp.
    assert_equal(ds1.time_warp(), 1)
    assert_equal(ds2.time_warp(), 1)

    mat = np.asarray([[0, 4], [3, 0]])
    merged12 = DurationSegment.merge(mat[0, 1], ds1, ds2)  # depot -> client
    merged21 = DurationSegment.merge(mat[1, 0], ds2, ds1)  # client -> depot

    # The order in which duration segments are merged matters, so although
    # these two segments cover the same clients, the total time warp is not
    # the same.
    assert_(merged12.time_warp() != merged21.time_warp())

    # Both segments start with 1 initial time warp. Going from depot -> client
    # happens at t = 5 - 1, and takes 4 time units to complete. So we arrive at
    # t = 8, which is three units after the time window closes. This adds 2
    # time warp to the segment, which, together with the initial time warps
    # makes for 2 + 1 + 1 = 4 total time warp.
    assert_equal(merged12.time_warp(), 4)

    # We can leave at the earliest at t = 3, the start of the client's time
    # window. We then arrive at t = 6, which adds one unit of time warp.
    # Combined with the initial time warp, this results in 1 + 1 + 1 = 3 units
    # of total time warp.
    assert_equal(merged21.time_warp(), 3)

    # This merged DS represents the route plan 0 -> 1 -> 1 -> 0. We leave 1
    # at t = 10 - 4, and travel takes no time. We do get the 1 unit of
    # existing time warp. We then travel to the depot, arriving at t = 9. This
    # is 4 time units after the time window closes, which adds 4 time warp
    # (plus the existing unit). So we get 4 + 1 + 1 + 4 = 10 time warp.
    merged = DurationSegment.merge(mat[1, 1], merged12, merged21)
    assert_equal(merged.time_warp(), 10)


def test_max_duration_argument():
    """
    Tests that the ``max_duration`` argument is evaluated correctly, and indeed
    increases the time warp if it's violated.
    """
    ds = DurationSegment(5, 0, 0, 0, 0)  # five duration

    assert_equal(ds.time_warp(), 0)  # default not duration limited
    assert_equal(ds.time_warp(max_duration=2), 3)
    assert_equal(ds.time_warp(max_duration=0), 5)


def test_OkSmall_with_time_warp(ok_small):
    """
    Tests a small example route using the OkSmall instance. In particular, we
    also check that duration does not include time warp.
    """
    vehicle_type = ok_small.vehicle_type(0)
    segments = [
        DurationSegment(
            duration=loc.service_duration if idx != 0 else 0,
            time_warp=0,
            start_early=loc.tw_early if idx > 0 else vehicle_type.tw_early,
            start_late=loc.tw_late if idx > 0 else vehicle_type.tw_late,
            release_time=loc.release_time if idx != 0 else 0,
            cum_duration=0,
            cum_time_warp=0,
        )
        for idx, loc in enumerate(ok_small.depots() + ok_small.clients())
    ]

    # Create the DS associated with route 0 -> 1 -> 3 -> 0.
    ds = segments[0]
    for frm, to in pairwise([0, 1, 3, 0]):
        mat = ok_small.duration_matrix(profile=0)
        ds = DurationSegment.merge(mat[frm, to], ds, segments[to])

    # First the route's duration. This depends on travel duration, service
    # time, and possible waiting time. We do not have waiting time on this
    # route. So all we need to determine is:
    #   Travel durations:
    #       - 0 -> 1: 1544
    #       - 1 -> 3: 1427
    #       - 3 -> 0: 2063
    #   Service times:
    #       - 1: 360
    #       - 3: 420
    assert_equal(ds.duration(), 1544 + 1427 + 2063 + 360 + 420)

    # But there is time warp as well, because 1's time window opens at 15600,
    # while 3's time window closes at 15300. So we leave 1 at 15600 + 360,
    # drive 1427 and arrive at 3 at 15600 + 360 + 1427 = 17387. We then warp
    # back in time to 15300, for 17387 - 15300 = 2087 time warp.
    assert_equal(ds.time_warp(), 2087)


def test_bug_fix_overflow_more_timewarp_than_duration():
    """
    This test exercises the issue identified in #588, when merging a duration
    segment that has more time warp than duration with another duration segment
    that has ``startLate = INT_MAX`` results in integer overflow.
    """
    ds1 = DurationSegment(9, 18, 0, 18, 0)
    assert_(ds1.duration() < ds1.time_warp())

    ds2 = DurationSegment(0, 0, 0, np.iinfo(np.int64).max, 0)
    assert_equal(ds2.start_late(), np.iinfo(np.int64).max)

    # ds1 has 9 duration and 18 time warp, which results in an arrival time of
    # -9 at ds2. Before enforcing non-negative arrival times, this would result
    # in an integer overflow when subtracting this arrival time from ds2's
    # start_late.
    ds = DurationSegment.merge(0, ds1, ds2)
    assert_equal(ds.time_warp(), 18)


def test_str():
    """
    Tests that the duration segment's string representation contains useful
    debugging information.
    """
    segment = DurationSegment(
        duration=2,
        time_warp=3,
        start_early=5,
        start_late=7,
        release_time=6,
        prev_end_late=3,
    )

    assert_("duration=5" in str(segment))  # incl prev_end_late to release_time
    assert_("time_warp=3" in str(segment))
    assert_("start_early=6" in str(segment))  # max(start_early, release_time)
    assert_("start_late=7" in str(segment))
    assert_("release_time=6" in str(segment))
    assert_("prev_end_late=3" in str(segment))


def test_finalise_back_with_time_warp_from_release_time():
    """
    Tests finalise_back() when there's time warp due to the release time.
    """
    # Release time is 75, which is after start_late of 70. So we have 5 time
    # warp from this.
    segment = DurationSegment(5, 0, 50, 70, 75)
    assert_equal(segment.start_early(), 70)
    assert_equal(segment.start_late(), 70)
    assert_equal(segment.release_time(), 75)
    assert_equal(segment.duration(), 5)
    assert_equal(segment.time_warp(), 5)  # due to release time

    # Tests that finalising does not affect duration and time warp.
    finalised = segment.finalise_back()
    assert_equal(finalised.duration(), 5)
    assert_equal(finalised.time_warp(), 5)

    # Finalised segments cannot start before the original segments, but are
    # not constrained in their latest start (since we could wait indefinitely).
    # We also track when the finalised segment would end at the earliest and
    # latest.
    assert_equal(finalised.start_early(), 75)
    assert_equal(finalised.start_late(), _INT_MAX)
    assert_equal(finalised.release_time(), 75)
    assert_equal(finalised.prev_end_late(), 75)


@pytest.mark.parametrize(
    ("prev_end_early", "prev_end_late", "exp_duration", "exp_time_warp"),
    [
        (95, 95, 5, 0),  # prev ends at 95, we start at 100. 5 wait duration.
        (95, 100, 0, 0),  # prev can end at 100, so we can immediately start.
        (120, 120, 0, 10),  # prev ends at 120; we must start by 110. 10 tw.
        (110, 120, 0, 0),  # prev can end at 110, so we can immediately start.
        (95, 120, 0, 0),  # we can start immediately between 100 and 110.
    ],
)
def test_duration_and_time_warp_from_prev_end_times(
    prev_end_early: int,
    prev_end_late: int,
    exp_duration: int,
    exp_time_warp: int,
):
    """
    Tests wait duration and/or time warp due to attributes associated with the
    early and latest start of the previous trip.
    """
    segment = DurationSegment(
        0,
        0,
        100,
        110,
        release_time=prev_end_early,
        prev_end_late=prev_end_late,
    )

    assert_equal(segment.duration(), exp_duration)
    assert_equal(segment.time_warp(), exp_time_warp)


@pytest.mark.parametrize(
    ("release_time", "exp_time_warp"),
    [
        (100, 0),  # we can feasibly start at 100, so no time warp.
        (110, 10),  # we need to wait until 110, but start_late=100.
    ],
)
def test_time_warp_from_release_time(release_time: int, exp_time_warp: int):
    """
    Tests that time warp due to a late release time of the current trip results
    in the expected amount of time warp.
    """
    segment = DurationSegment(0, 0, 0, 100, release_time)
    assert_equal(segment.start_late(), 100)
    assert_equal(segment.time_warp(), exp_time_warp)


def test_finalise_front():
    """
    Tests that finalise_front() correctly finalises the segment.
    """
    segment = DurationSegment(5, 5, 40, 50, 50)
    assert_equal(segment.duration(), 5)
    assert_equal(segment.time_warp(), 5)

    assert_equal(segment.start_early(), 50)
    assert_equal(segment.start_late(), 50)
    assert_equal(segment.release_time(), 50)

    # Test that finalising does not affect duration and time warp.
    finalised = segment.finalise_front()
    assert_equal(finalised.duration(), 5)
    assert_equal(finalised.time_warp(), 5)

    # Same start_early and start_late as segment, but no release time.
    assert_equal(finalised.start_early(), 50)
    assert_equal(finalised.start_late(), 50)
    assert_equal(finalised.release_time(), 0)


def test_repeated_merge_and_finalise_back():
    """
    Tests finalise_back() in a common multi-trip scenario, where we merge
    several duration segments.
    """
    # We model a route consisting of two trips with duration segments segment1
    # and segment2.
    segment1 = DurationSegment(45, 0, 30, 50, 50)
    segment2 = DurationSegment(50, 0, 70, 110, 100)

    # segment1 finalises at a reload depot, so we need to finalise at the end.
    finalised1 = segment1.finalise_back()
    assert_equal(finalised1.start_early(), 95)
    assert_equal(finalised1.start_late(), _INT_MAX)
    assert_equal(finalised1.release_time(), 95)
    assert_equal(finalised1.prev_end_late(), 95)

    # Next we execute the second trip, so we merge segment2.
    merged = DurationSegment.merge(0, finalised1, segment2)
    assert_equal(merged.duration(), 100)  # including 5 wait time
    assert_equal(merged.start_early(), 100)
    assert_equal(merged.start_late(), 110)
    assert_equal(merged.release_time(), 100)

    # While the second trip may start between [100, 110] without increasing the
    # trip duration, waiting beyond 100 will increase the route duration. Thus,
    # there is zero slack.
    assert_equal(merged.slack(), 0)

    # Return to the end depot. Duration should not change, but we do need to
    # make sure the end times are correct.
    finalised2 = merged.finalise_back()
    assert_equal(finalised2.duration(), 100)
    assert_equal(finalised2.start_early(), 150)
    assert_equal(finalised2.start_late(), _INT_MAX)
    assert_equal(finalised2.release_time(), 150)
    assert_equal(finalised2.prev_end_late(), 150)


def test_finalise_nonzero_route_slack():
    """
    Tests that finalising segments with loose time windows results in positive
    route slack.
    """
    segment1 = DurationSegment(0, 0, 0, 100, 0)
    segment2 = DurationSegment(0, 0, 50, 75, 0)

    finalised1 = segment1.finalise_back()
    assert_equal(finalised1.release_time(), 0)
    assert_equal(finalised1.prev_end_late(), 100)
    assert_equal(finalised1.slack(), 100)

    merged = DurationSegment.merge(0, finalised1, segment2)
    finalised2 = merged.finalise_back()
    assert_equal(finalised2.release_time(), 50)
    assert_equal(finalised2.prev_end_late(), 75)
    assert_equal(finalised2.slack(), 25)


def test_end_early_and_late():
    """
    Tests the end_early() and end_late() computations for a small example.
    """
    segment = DurationSegment(40, 30, 10, 20, 0, 15, 5)
    assert_equal(segment.start_early(), 10)
    assert_equal(segment.start_late(), 20)
    assert_equal(segment.duration(), 40 + 15)  # includes cumulative
    assert_equal(segment.time_warp(), 30 + 5)  # includes cumulative
    assert_equal(segment.end_early(), 20)  # ignores cumulative
    assert_equal(segment.end_late(), 30)  # ignores cumulative


def test_finalise_back_front_merge_same_thing():
    """
    Tests that finalising either the front or the back and then merging on the
    appropriate side results in segments with the same duration and time warp
    attributes.
    """
    ds1 = DurationSegment(50, 0, 70, 110, 100)
    ds2 = DurationSegment(45, 0, 30, 50, 50)

    # Finalise the first segment at its back, then merge. Or finalise the
    # second segment at the front and then merge. This should result in the
    # same segment w.r.t. time warp and duration.
    finalise_back = DurationSegment.merge(0, ds1.finalise_back(), ds2)
    finalise_front = DurationSegment.merge(0, ds1, ds2.finalise_front())
    assert_equal(finalise_back.time_warp(), finalise_front.time_warp())
    assert_equal(finalise_back.duration(), finalise_front.duration())
