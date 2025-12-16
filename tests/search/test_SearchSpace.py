import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp.search import NeighbourhoodParams, compute_neighbours
from pyvrp.search._search import SearchSpace


@pytest.mark.parametrize("size", [1, 2, 3, 4, 6, 7])  # num_clients + 1 == 5
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


def test_raises_when_neighbourhood_contains_self_or_depot(ok_small):
    """
    Tests that the search space raises when the granular neighbourhood contains
    the depot (for any client) or the client is in its own neighbourhood.
    """
    neighbours = [[], [2], [3], [4], [0]]  # 4 has depot as neighbour
    with assert_raises(RuntimeError):
        SearchSpace(ok_small, neighbours)

    neighbours = [[], [1], [3], [4], [1]]  # 1 has itself as neighbour
    with assert_raises(RuntimeError):
        SearchSpace(ok_small, neighbours)


@pytest.mark.parametrize(
    (
        "weight_wait_time",
        "weight_time_warp",
        "num_neighbours",
        "symmetric_proximity",
        "symmetric_neighbours",
    ),
    [
        (20, 20, 10, True, False),
        (20, 20, 10, True, True),
        # From original c++ implementation
        # (18, 20, 34, False),
        (18, 20, 34, True, True),
    ],
)
def test_set_get_neighbours(
    rc208,
    weight_wait_time: int,
    weight_time_warp: int,
    num_neighbours: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
):
    """
    Tests setting and getting neighbours.
    """
    params = NeighbourhoodParams(num_neighbours=1)
    prev_neighbours = compute_neighbours(rc208, params)
    search_space = SearchSpace(rc208, prev_neighbours)

    params = NeighbourhoodParams(
        weight_wait_time,
        weight_time_warp,
        num_neighbours,
        symmetric_proximity,
        symmetric_neighbours,
    )
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
