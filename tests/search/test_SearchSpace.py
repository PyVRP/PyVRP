import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import RandomNumberGenerator
from pyvrp.search import NeighbourhoodParams, compute_neighbours
from pyvrp.search._search import SearchSpace
from tests.helpers import make_search_route


@pytest.mark.parametrize("size", [1, 2, 3, 5, 6, 7])  # num_clients == 4
def test_raises_when_neighbourhood_dimensions_do_not_match(ok_small, size):
    """
    Tests that the search space raises when the neighbourhood size does not
    correspond to the problem dimensions.
    """
    # Each of the given sizes is either smaller than or bigger than desired.
    neighbours = [[] for _ in range(size)]
    with assert_raises(RuntimeError):
        SearchSpace(ok_small, neighbours)

    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    with assert_raises(RuntimeError):
        search_space.neighbours = neighbours


def test_raises_when_neighbourhood_contains_self(ok_small):
    """
    Tests that the search space raises when a client is in its own
    neighbourhood.
    """
    neighbours = [[0], [2], [3], [0]]  # 0 has itself as neighbour
    with assert_raises(RuntimeError):
        SearchSpace(ok_small, neighbours)


@pytest.mark.parametrize(
    ("num_neighbours", "symmetric_proximity"),
    [
        (10, True),
        # From original c++ implementation
        (34, False),
    ],
)
def test_set_get_neighbours(
    rc208,
    num_neighbours: int,
    symmetric_proximity: bool,
):
    """
    Tests setting and getting neighbours.
    """
    params = NeighbourhoodParams(num_neighbours=1)
    prev_neighbours = compute_neighbours(rc208, params)
    search_space = SearchSpace(rc208, prev_neighbours)

    params = NeighbourhoodParams(20, num_neighbours, symmetric_proximity)
    neighbours = compute_neighbours(rc208, params)

    # Test that before we set neighbours we don't have same
    assert_(search_space.neighbours != neighbours)

    # Test after we set we have the same
    search_space.neighbours = neighbours
    assert_equal(search_space.neighbours, neighbours)

    # Check that the bindings make a copy (in both directions)
    assert_(search_space.neighbours is not neighbours)
    search_space_neighbours = search_space.neighbours
    search_space_neighbours[1] = []
    assert_(search_space.neighbours != search_space_neighbours)
    assert_equal(search_space.neighbours, neighbours)
    neighbours[1] = []
    assert_(search_space.neighbours != neighbours)


def test_get_neighbours(ok_small):
    """
    Tests getting the SearchSpace's full granular neighbourhood, and
    client-specific neighbourhoods.
    """
    neighbours = [[1, 2], [2, 3], [0, 1], [1, 2]]

    search_space = SearchSpace(ok_small, neighbours)
    assert_equal(search_space.neighbours, neighbours)

    for idx in range(ok_small.num_clients):
        assert_equal(neighbours[idx], search_space.neighbours_of(idx))


def test_promising(ok_small):
    """
    Tests marking clients as promising.
    """
    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))

    for client in range(ok_small.num_clients):
        # The client does not start off promising.
        assert_(not search_space.is_promising(client))

        # But it is after being marked promising.
        search_space.mark_promising(client)
        assert_(search_space.is_promising(client))


def test_all_promising(ok_small):
    """
    Tests marking and unmarking all clients as promising.
    """
    clients = range(ok_small.num_clients)

    # Initially no clients start off promising.
    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    assert_(not any(search_space.is_promising(client) for client in clients))

    # But after marking all as promising, they should all indeed be promising.
    search_space.mark_all_promising()
    assert_(all(search_space.is_promising(client) for client in clients))

    # After unmarking all, no client should be promising.
    search_space.unmark_all_promising()
    assert_(not any(search_space.is_promising(client) for client in clients))


@pytest.mark.parametrize(
    ("mark", "exp_marked", "exp_unmarked"),
    [
        (0, [0], [1, 2, 3]),  # start depot
        (1, [0, 1], [2, 3]),
        (2, [0, 1, 2], [3]),
        (3, [1, 2, 3], [0]),
        (4, [2, 3], [0, 1]),
        (5, [3], [0, 1, 2]),  # end depot
    ],
)
def test_node_promising(
    ok_small,
    mark: int,
    exp_marked: list[int],
    exp_unmarked: list[int],
):
    """
    Tests marking nodes (and their client neighbours) as promising.
    """
    route = make_search_route(ok_small, ["C0", "C1", "C2", "C3"])

    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    search_space.mark_promising(route[mark])

    for exp in exp_marked:
        assert_(search_space.is_promising(exp))

    for exp in exp_unmarked:
        assert_(not search_space.is_promising(exp))


def test_search_order_and_shuffle(ok_small_two_profiles):
    """
    Tests that the search order begins with an unshuffled default, and then
    randomises with each shuffle call.
    """
    data = ok_small_two_profiles
    search_space = SearchSpace(data, compute_neighbours(data))

    # Initially we have an unshuffled, default search order.
    assert_equal(search_space.client_order(), [0, 1, 2, 3])  # 4 clients
    assert_equal(search_space.veh_type_order(), [(0, 0), (1, 3)])  # 2 types

    rng = RandomNumberGenerator(seed=1821)
    search_space.shuffle(rng)

    # After shuffling, the search order has changed, but is still fixed: it
    # does not change again until we do another shuffle.
    assert_equal(search_space.client_order(), [0, 2, 1, 3])
    assert_equal(search_space.client_order(), [0, 2, 1, 3])  # again to check
    assert_equal(search_space.veh_type_order(), [(1, 3), (0, 0)])

    # Shuffling again changes the search order.
    search_space.shuffle(rng)
    assert_equal(search_space.client_order(), [3, 2, 1, 0])
