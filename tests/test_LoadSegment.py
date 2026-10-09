import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp._pyvrp import LoadSegment

_INT_MAX = np.iinfo(np.int64).max


@pytest.mark.parametrize(
    ("initial", "delta", "increase"),
    [(1, 2, 3), (0, 0, 0), (_INT_MAX, _INT_MAX, _INT_MAX)],
)
def test_attribute_getters(initial: int, delta: int, increase: int):
    """
    Tests that the attribute member functions return the passed in values.
    """
    load_segment = LoadSegment(initial, delta, increase)
    assert_equal(load_segment.initial(), initial)
    assert_equal(load_segment.delta(), delta)
    assert_equal(load_segment.increase(), increase)


@pytest.mark.parametrize(
    ("first", "second", "exp_init", "exp_increase", "exp_load"),
    [
        (
            LoadSegment(initial=5, delta=0, increase=3),
            LoadSegment(initial=3, delta=0, increase=8),
            5 + 3,  # initial loads
            8,  # = max(increase1, delta1 + increase2) = max(3, 0 + 8) = 8
            16,  # initial + increase
        ),
        (
            LoadSegment(initial=3, delta=4, increase=11),
            LoadSegment(initial=5, delta=0, increase=8),
            3 + 5,  # initial loads
            12,  # = max(increase1, delta1 + increase2) = max(11, 4 + 8) = 12
            20,  # initial + increase
        ),
    ],
)
def test_merge_two(
    first: LoadSegment,
    second: LoadSegment,
    exp_init: int,
    exp_increase: int,
    exp_load: int,
):
    """
    Tests merging two load segments.
    """
    merged = LoadSegment.merge(first, second)
    assert_equal(merged.initial(), exp_init)
    assert_equal(merged.increase(), exp_increase)
    assert_equal(merged.load(), exp_load)

    assert_equal(merged.excess_load(0), exp_load)
    assert_equal(merged.excess_load(capacity=exp_load), 0)


def test_excess_load_capacity():
    """
    Tests that excess load is correctly evaluated and merged.
    """
    before = LoadSegment(5, 0, 0, excess_load=30)
    after = LoadSegment(2, 0, 0, excess_load=5)
    merged = LoadSegment.merge(before, after)

    # There's seven load on this segment, but 30 excess load from some part of
    # the route executed before the last return to the depot, and 5 excess load
    # from part of the route executed after the next return to the depot.
    assert_equal(merged.load(), 7)
    assert_equal(merged.excess_load(7), 35)
    assert_equal(merged.excess_load(0), 42)


@pytest.mark.parametrize(
    ("capacity", "exp_excess"),
    [(10, 20), (5, 20), (0, 25)],
)
def test_finalise(capacity: int, exp_excess: int):
    """
    Tests that excess load is correctly tracked by finalised load segments.
    """
    segment = LoadSegment(5, 0, 0, 20)  # 20 excess, and 5 initial segment load
    finalised = segment.finalise(capacity)

    # Finalised segments track cumulative excess load - the rest resets.
    assert_equal(finalised.initial(), 0)
    assert_equal(finalised.delta(), 0)
    assert_equal(finalised.increase(), 0)
    assert_equal(finalised.load(), 0)
    assert_equal(finalised.excess_load(capacity), exp_excess)


def test_str():
    """
    Tests that the load segment's string representation contains useful
    debugging information.
    """
    segment = LoadSegment(initial=2, delta=3, increase=5, excess_load=8)
    print(segment)
    assert_("initial=2" in str(segment))
    assert_("delta=3" in str(segment))
    assert_("increase=5" in str(segment))
    assert_("load=7" in str(segment))  # initial + increase = 2 + 5 = 7
    assert_("excess_load=8" in str(segment))


def test_mixed_clients_and_shipments():
    """
    Tests that the load segment's attributes are correct when evaluating a
    segment containing a mix of clients and shipments.
    """
    # Pickup shipments 1 and 2. Shipment 1 has load 5, shipment 2 load 3.
    pickup1 = LoadSegment(0, 5, 5)
    pickup2 = LoadSegment(0, 3, 3)
    merged = LoadSegment.merge(pickup1, pickup2)
    assert_equal(merged.load(), 8)

    # Now add a delivery for client 1, adding 5 load from the depot to this
    # client. That brings the total load to 13.
    client1 = LoadSegment(5, 0, 0)
    merged = LoadSegment.merge(merged, client1)
    assert_equal(merged.load(), 13)

    # Add a pickup for client 2, adding five load from this client to the
    # depot. That should not increase load further.
    client2 = LoadSegment(0, 0, -5, 0)
    merged = LoadSegment.merge(merged, client2)
    assert_equal(merged.load(), 13)

    # Now deliver the first shipment.
    delivery1 = LoadSegment(0, 0, -5)
    merged = LoadSegment.merge(merged, delivery1)
    assert_equal(merged.load(), 13)

    # Now finalise the segment. This should clear all client load, but not
    # affect shipment load since those do not go through depots and are thus
    # not finalised.
    finalised = merged.finalise(capacity=11)
    assert_equal(finalised.load(), 0)
    assert_equal(finalised.excess_load(capacity=11), 2)
