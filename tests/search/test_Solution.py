import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

import pyvrp
from pyvrp import Client, CostEvaluator, Depot, ProblemData, Route, VehicleType
from pyvrp import Solution as PyVRPSolution
from pyvrp.search import compute_neighbours
from pyvrp.search._search import SearchSpace, Solution


def test_load_unload(ok_small):
    """
    Tests that loading and then unloading an unchanged solution returns the
    same original solution.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[1, 2], [3, 4]])

    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)
    assert_equal(search_sol.unload(), pyvrp_sol)


def test_loading_twice_in_a_row(ok_small):
    """
    When loading the same solution we re-use the representation of the previous
    solution as much as possible. Here we test that loading the same solution
    twice in a row (which triggers that re-use) still produces the same
    solution when unloading.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[1, 2], [3, 4]])

    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)
    search_sol.load(pyvrp_sol)
    assert_equal(search_sol.unload(), pyvrp_sol)


def test_nodes_routes_access(ok_small):
    """
    Tests nodes and routes access.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[1, 2], [3, 4]])
    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)

    # There should be #locations nodes, and #vehicles routes.
    assert_equal(len(search_sol.nodes), ok_small.num_locations)
    assert_equal(len(search_sol.routes), ok_small.num_vehicles)

    for client in [1, 2]:  # [1, 2] are in the first route
        assert_equal(search_sol.nodes[client].client, client)
        assert_equal(search_sol.nodes[client].route, search_sol.routes[0])

    for client in [3, 4]:  # [3, 4] are in the second route
        assert_equal(search_sol.nodes[client].client, client)
        assert_equal(search_sol.nodes[client].route, search_sol.routes[1])


def test_insert_required(ok_small):
    """
    Tests that inserting clients can fail if inserting is too expensive and
    the insert is not required.
    """
    data = ok_small
    search_space = SearchSpace(data, compute_neighbours(data))
    cost_eval = CostEvaluator([0], 0, 0)

    # Start with an empty solution and try to insert the first client without
    # requiring an insert. Inserting should fail: it's not worth it, since the
    # client has no prize. However, inserting should succeed when required.
    sol = Solution(data)
    assert_(not sol.insert(sol.nodes[1], search_space, cost_eval, False))
    assert_(sol.insert(sol.nodes[1], search_space, cost_eval, True))


def test_distance_and_duration_cost(ok_small):
    """
    Tests the Solution's distance_cost() and duration_cost() methods.
    """
    veh_type = ok_small.vehicle_type(0).replace(unit_duration_cost=1)
    data = ok_small.replace(vehicle_types=[veh_type])

    sol = Solution(data)
    pyvrp_sol = PyVRPSolution(data, [[1, 2], [3, 4]])
    sol.load(pyvrp_sol)

    assert_(sol.distance_cost() > 0)
    assert_equal(sol.distance_cost(), pyvrp_sol.distance_cost())
    assert_equal(
        sol.distance_cost(),
        sum(route.distance_cost() for route in sol.routes),
    )

    assert_(sol.duration_cost() > 0)
    assert_equal(sol.duration_cost(), pyvrp_sol.duration_cost())
    assert_equal(
        sol.duration_cost(),
        sum(route.duration_cost() for route in sol.routes),
    )


@pytest.mark.parametrize(
    ("assignment", "expected"), [((0, 0), 0), ((0, 1), 10), ((1, 1), 20)]
)
def test_fixed_vehicle_cost(
    ok_small, assignment: tuple[int, int], expected: int
):
    """
    Tests the Solution's fixed_vehicle_cost() method.
    """
    # First vehicle type is free, second costs 10 per vehicle. The solution
    # should be able to track this.
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=[10], fixed_cost=0),
            VehicleType(2, capacity=[10], fixed_cost=10),
        ],
    )

    routes = [Route(data, [1], assignment[0]), Route(data, [2], assignment[1])]
    pyvrp_sol = PyVRPSolution(data, routes)

    sol = Solution(data)
    sol.load(pyvrp_sol)

    assert_equal(sol.fixed_vehicle_cost(), expected)
    assert_equal(sol.fixed_vehicle_cost(), pyvrp_sol.fixed_vehicle_cost())


def test_excess_load(ok_small):
    """
    Tests the Solution's excess_load() method.
    """
    sol = Solution(ok_small)
    sol.load(PyVRPSolution(ok_small, [[1, 2, 3, 4]]))

    needed = sum(client.delivery[0] for client in ok_small.clients())
    available = ok_small.vehicle_type(0).capacity[0]
    assert_equal(needed, 18)
    assert_equal(available, 10)
    assert_equal(sol.excess_load(), [needed - available])


def test_excess_load_no_dims():
    """
    Tests the Solution's excess_load() method on an instance without load
    dimensions.
    """
    data = ProblemData(
        depots=[Depot(x=0, y=0)],
        clients=[Client(x=0, y=0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((2, 2), dtype=int)],
        duration_matrices=[np.zeros((2, 2), dtype=int)],
    )

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[1]]))
    assert_equal(sol.excess_load(), [])


def test_excess_distance(ok_small):
    """
    Tests the Solution's excess_distance() method.
    """
    vehicle_type = VehicleType(3, capacity=[10], max_distance=5_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[1, 2]]))

    assert_equal(sol.distance_cost(), 5501)  # unit cost, so also distance()
    assert_equal(sol.routes[0].distance(), 5501)

    assert_equal(sol.excess_distance(), 501)
    assert_equal(sol.routes[0].excess_distance(), 501)


def test_time_warp(ok_small):
    """
    Tests the Solution's time_warp() method.
    """
    sol = Solution(ok_small)
    sol.load(PyVRPSolution(ok_small, [[1, 2, 3, 4]]))

    # Visiting all clients of this instance in order always yields 3_633 time
    # warp because visiting 1 before 3 is infeasible.
    assert_equal(sol.time_warp(), 3_633)

    # Swapping 1 and 3 is time-feasible.
    sol.load(PyVRPSolution(ok_small, [[3, 2, 1, 4]]))
    assert_equal(sol.time_warp(), 0)


@pytest.mark.parametrize(
    ("visits", "exp_uncollected"),
    [
        ([], 35),  # all uncollected
        ([1], 35),  # first client has no prize
        ([1, 2], 20),
        ([1, 2, 3, 4], 0),  # all collected
    ],
)
def test_uncollected_prizes(
    ok_small_prizes,
    visits: list[int],
    exp_uncollected: int,
):
    """
    Tests the Solution's uncollected_prizes() method.
    """
    sol = Solution(ok_small_prizes)
    sol.load(PyVRPSolution(ok_small_prizes, [visits] if visits else []))
    assert_equal(sol.uncollected_prizes(), exp_uncollected)
