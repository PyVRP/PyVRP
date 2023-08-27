from numpy.testing import assert_equal

from pyvrp import CostEvaluator
from pyvrp.search._search import Node, Route, insert_cost, remove_cost
from pyvrp.tests.helpers import read


def test_insert_cost_zero_when_not_allowed():
    """
    Tests that insert_cost() returns zero when a move is not possible. This is
    the only case where it shortcuts to return a non-negative delta cost.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    route = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route.append(Node(loc=loc))
    route.update()

    # Inserting the depot is not possible.
    assert_equal(insert_cost(route[0], route[1], data, cost_eval), 0)
    assert_equal(insert_cost(route[3], route[2], data, cost_eval), 0)

    # Inserting after a node that's not in a route is not possible.
    assert_equal(insert_cost(route[1], Node(loc=3), data, cost_eval), 0)


def test_insert_cost():
    """
    Tests that the insert_cost() method works correctly on a few basic cases.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    route = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route.append(Node(loc=loc))
    route.update()

    # This adds arcs 1 -> 4 -> 2, and removes arcs 1 -> 2. The added distances
    # is 1593 + 1090, the removed distance 1992. This also adds 5 additional
    # demand, which exceeds the vehicle capacity by 5, so we get an additional
    # load penalty of 5. There is no time warp. Total delta cost:
    #     1593 + 1090 - 1992 + 5 = 696.
    assert_equal(insert_cost(Node(loc=4), route[1], data, cost_eval), 696)

    # +5 load penalty, and delta dist is 1090 + 1475 - 1965 = 600. So total
    # delta cost is 605 (again no time warp).
    assert_equal(insert_cost(Node(loc=4), route[2], data, cost_eval), 605)

    # Now we do have some time warp changes. +3 load penalty, delta dist is
    # 1427 + 647 - 1992 = 82, but time warp increases because we cannot get to
    # client 3 quick enough from client 1. In particular, we can arrive at
    # client 1 at 15600, service for 360, and then travel to client 3 for
    # duration 1427. Then we arrive at client 3 at 17387, which is after its
    # closing time window of 15300. So we add 17387 - 15300 = 2087 time warp.
    # Tallying it all up, total delta cost:
    #      3 + 82 + 2087 = 2172.
    assert_equal(insert_cost(Node(loc=3), route[1], data, cost_eval), 2172)

    # +3 load penalty, delta dist is 621 + 2063 - 1965 = 719. Time warp
    # increases because we have to visit clients 1 and 2 before we visit
    # client 3. We get to client 1 at 15600, service for 360, travel to client
    # 2 for 1992, service for 360, and then travel to client 3 for 621, where
    # we arrive at 18933, after its closing time window of 15300. This adds
    # 18933 - 15300 = 3633 time warp. Tallying it all up, total delta cost:
    #     3 + 719 + 3633 = 4355.
    assert_equal(insert_cost(Node(loc=3), route[2], data, cost_eval), 4355)


def test_remove_cost_zero_when_not_allowed():
    """
    Tests that remove_cost() returns zero when a move is not possible. This is
    the only case where it shortcuts to return a non-negative delta cost.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    route = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route.append(Node(loc=loc))
    route.update()

    # Removing the depot is not possible.
    assert_equal(remove_cost(route[0], data, cost_eval), 0)
    assert_equal(remove_cost(route[3], data, cost_eval), 0)

    # Removing a node that's not in a route is not possible.
    assert_equal(remove_cost(Node(loc=3), data, cost_eval), 0)


def test_remove():
    """
    Tests that the remove_cost() method works correctly on a few basic cases.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    route = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route.append(Node(loc=loc))
    route.update()

    # Purely distance. Removes arcs 0 -> 1 -> 2, adds arc 0 -> 2. This change
    # has delta distance of 1944 - 1544 - 1992 = -1592.
    assert_equal(remove_cost(route[1], data, cost_eval), -1592)

    # Purely distance. Removes arcs 1 -> 2 -> 0, adds arcs 1 -> 0. This change
    # has delta distance of 1726 - 1992 - 1965 = -2231.
    assert_equal(remove_cost(route[2], data, cost_eval), -2231)
