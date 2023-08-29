from typing import List

import pytest
from numpy.testing import assert_equal

from pyvrp.search import SwapRoutes
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import read


@pytest.mark.parametrize(
    ("visits1", "visits2"),
    [
        ([], []),  # both empty
        ([1], []),  # first non-empty, second empty
        ([], [1]),  # first empty, second non-empty
        ([1], [2, 3]),  # both non-empty but unequal length
        ([2, 3], [1]),  # both non-empty but unequal length (flipped)
        ([2, 3], [1, 4]),  # both non-empty equal length
    ],
)
def test_apply(visits1: List[int], visits2: List[int]):
    """
    Tests that applying SwapRoutes to two different routes indeed exchanges
    the visits.
    """
    data = read("data/OkSmall.txt")

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in visits1:
        route1.append(Node(loc=loc))

    route2 = Route(data, idx=1, vehicle_type=0)
    for loc in visits2:
        route2.append(Node(loc=loc))

    route1.update()
    route2.update()

    # Before calling apply, route1 visits the clients in visits1, and route2
    # visits the clients in visits2.
    assert_equal(visits1, [node.client for node in route1])
    assert_equal(visits2, [node.client for node in route2])

    op = SwapRoutes(data)
    op.apply(route1, route2)

    # But after apply, the visits are now swapped.
    assert_equal(visits2, [node.client for node in route1])
    assert_equal(visits1, [node.client for node in route2])
