import pickle

import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    Model,
    PenaltyManager,
    PenaltyParams,
    Solution,
    Statistics,
    VehicleType,
)
from pyvrp.minimise_fleet import _lower_bound
from pyvrp.search._search import Solution as SearchSolution
from pyvrp.stop import MaxIterations
from tests.helpers import read


@pytest.mark.parametrize(
    "make",
    [
        lambda value: Location(value, 0),
        lambda value: Client(0, delivery=[value]),
        lambda value: Client(0, service_duration=value),
        lambda value: Client(0, prize=value),
        lambda value: VehicleType(max_distance=value),
    ],
)
@pytest.mark.parametrize("value", [0.0, 1.0, 1e6])
def test_measure_equality_tolerates_rounding(make, value):
    scale = max(1.0, value)
    assert_(make(value) == make(value + scale * 5e-10))
    assert_(make(value + scale * 5e-10) == make(value))
    assert_(make(value) != make(value + scale * 2e-9))


@pytest.mark.parametrize("value", [1.0, 1e6])
def test_measure_ordering_tolerates_rounding(value):
    # Validation uses the same comparisons as feasibility and search.
    Client(0, tw_early=value + value * 5e-10, tw_late=value)
    with pytest.raises(ValueError):
        Client(0, tw_early=value + value * 2e-9, tw_late=value)


@pytest.mark.parametrize(
    ("first", "second", "equal"),
    [
        (np.inf, np.inf, True),
        (-np.inf, -np.inf, True),
        (np.inf, -np.inf, False),
        (np.inf, 1.0, False),
        (-np.inf, 1.0, False),
        (np.nan, np.nan, False),
        (np.nan, np.inf, False),
        (np.nan, 1.0, False),
    ],
)
def test_measure_equality_nonfinite_values(first, second, equal):
    assert_equal(Location(first, 0) == Location(second, 0), equal)
    assert_equal(Location(second, 0) == Location(first, 0), equal)


def test_measure_tolerance_near_zero():
    evaluator = CostEvaluator([1], 1, 1)
    assert_allclose(evaluator.load_penalty(1.0 + 5e-10, 1.0, 0), 0)
    assert_allclose(evaluator.load_penalty(1.0 + 2e-9, 1.0, 0), 2e-9)

    # Tolerance affects comparisons, not the stored input values.
    assert_allclose(Client(0, delivery=[5e-10]).delivery, [5e-10], atol=0)


def test_equal_solutions_have_equal_hashes(ok_small):
    matrices = [matrix.copy() for matrix in ok_small.distance_matrices()]
    matrices[0][0, 1] += 5e-10
    perturbed = ok_small.replace(distance_matrices=matrices)
    original = Solution(ok_small, [[0, 1], [2, 3]])
    other = Solution(perturbed, [[2, 3], [0, 1]])
    assert_(original == other)
    assert_equal(hash(original), hash(other))
    assert_(other in {original})


@pytest.fixture
def fractional_data():
    model = Model()
    locations = [model.add_location(idx, 0) for idx in range(4)]
    model.add_depot(locations[0])
    model.add_vehicle_type(
        capacity=0.3,
        max_distance=0.6,
        fixed_cost=0.2,
        unit_distance_cost=0.7,
        unit_duration_cost=0.5,
    )
    for idx, location in enumerate(locations[1:], 1):
        model.add_client(location, delivery=0.1, tw_late=idx / 10)

    for frm in locations:
        for to in locations:
            travel = abs(frm.x - to.x) / 10
            model.add_edge(frm, to, distance=travel, duration=travel)

    return model.data()


def test_fractional_solution_and_search_route(fractional_data):
    """
    Summation rounding must not make a route at its limits infeasible.
    """
    solution = Solution(fractional_data, [[0, 1, 2]])
    search_solution = SearchSolution(fractional_data)
    search_solution.load(solution)

    for obj in (solution, solution.routes()[0], search_solution.routes[0]):
        assert_(obj.is_feasible())
        assert_allclose(obj.distance(), 0.6)
        assert_allclose(obj.duration(), 0.6)
        assert_allclose(obj.excess_load(), [0], atol=1e-12)
        assert_allclose(obj.excess_distance(), 0, atol=1e-12)
        assert_allclose(obj.time_warp(), 0, atol=1e-12)

    evaluator = CostEvaluator([0.7], 0.5, 0.3)
    assert_allclose(evaluator.cost(solution), 0.92)
    assert_allclose(evaluator.penalised_cost(solution), 0.92)
    assert_allclose(evaluator.load_penalty(0.4, 0.3, 0), 0.07)
    assert_allclose(evaluator.tw_penalty(0.1), 0.05)
    assert_allclose(evaluator.dist_penalty(0.7, 0.6), 0.03)


@pytest.mark.parametrize(
    ("capacity", "max_distance", "violation"),
    [(0.29, 0.6, "excess_load"), (0.3, 0.59, "excess_distance")],
)
def test_fractional_violations(
    fractional_data, capacity, max_distance, violation
):
    """
    Real fractional violations are still detected and penalised.
    """
    vehicle = fractional_data.vehicle_type(0).replace(
        capacity=[capacity], max_distance=max_distance
    )
    data = fractional_data.replace(vehicle_types=[vehicle])
    solution = Solution(data, [[0, 1, 2]])
    search_solution = SearchSolution(data)
    search_solution.load(solution)

    for obj in (solution, solution.routes()[0], search_solution.routes[0]):
        assert_(not obj.is_feasible())
        assert_allclose(getattr(obj, violation)(), 0.01)


def test_fractional_local_search(fractional_data):
    """
    Search terminates with fractional costs and tight fractional constraints.
    """
    model = Model.from_data(fractional_data)
    result = model.solve(MaxIterations(5), display=False)
    assert_(result.is_feasible())
    assert_allclose(result.cost(), 0.92)


@pytest.mark.parametrize("unit_duration_cost", [0, 0.5])
def test_infinite_upper_bounds(fractional_data, unit_duration_cost):
    """
    Unbounded windows and vehicle limits leave actual route statistics finite.
    """
    vehicle = fractional_data.vehicle_type(0).replace(
        tw_late=np.inf,
        start_late=np.inf,
        shift_duration=np.inf,
        max_distance=np.inf,
        unit_duration_cost=unit_duration_cost,
    )
    data = fractional_data.replace(
        clients=[
            Client(client.location, delivery=client.delivery, tw_late=np.inf)
            for client in fractional_data.clients()
        ],
        depots=[Depot(0, tw_late=np.inf)],
        vehicle_types=[vehicle],
    )
    assert_(not data.has_time_windows())

    result = Model.from_data(data).solve(MaxIterations(5), display=False)
    assert_(result.is_feasible())
    assert_allclose(result.cost(), 0.62 + 0.6 * unit_duration_cost)

    solution = result.best
    search_solution = SearchSolution(data)
    search_solution.load(solution)
    for route in (solution.routes()[0], search_solution.routes[0]):
        assert_allclose(route.distance(), 0.6)
        assert_allclose(route.duration(), 0.6)
        assert_allclose(route.time_warp(), 0)
        assert_allclose(route.overtime(), 0)
        assert_allclose(route.excess_distance(), 0)


def test_fractional_fleet_bound(fractional_data):
    """
    Capacity bounds work below one and tolerate summation rounding.
    """
    assert_equal(_lower_bound(fractional_data), 1)
    vehicle = fractional_data.vehicle_type(0).replace(capacity=[0.15])
    data = fractional_data.replace(vehicle_types=[vehicle])
    assert_equal(_lower_bound(data), 2)


def test_read_without_rounding():
    """
    The default reader retains fractional Euclidean distances.
    """
    data = read("data/RC208.vrp", round_func="none")
    expected = np.sqrt((40 - 25) ** 2 + (85 - 50) ** 2)
    assert_allclose(data.distance_matrix(0)[0, 1], expected)
    assert_allclose(data.duration_matrix(0)[0, 1], expected)
    assert_equal(data.distance_matrix(0).dtype, np.float64)


def test_fractional_penalty_registrations(fractional_data):
    """
    Penalty updates agree with feasibility checks near constraint bounds.
    """
    solution = Solution(fractional_data, [[0, 1, 2]])
    params = PenaltyParams(solutions_between_updates=1, penalty_decrease=0.5)
    manager = PenaltyManager(([1], 1, 1), params)
    manager.register(solution)
    loads, time_warp, distance = manager.penalties()
    assert_allclose(loads, [0.5])
    assert_allclose([time_warp, distance], [0.5, 0.5])


def test_fractional_serialisation(fractional_data, tmp_path):
    """
    Pickling and CSV statistics retain fractional data and costs.
    """
    data = pickle.loads(pickle.dumps(fractional_data))
    solution = Solution(data, [[0, 1, 2]])
    restored = pickle.loads(pickle.dumps(solution))
    assert_allclose(restored.distance(), 0.6)
    assert_allclose(data.client(0).delivery, [0.1])
    assert_equal(data.distance_matrix(0).dtype, np.float64)

    stats = Statistics()
    stats.collect(solution, solution, solution, CostEvaluator([1], 1, 1))
    path = tmp_path / "fractional.csv"
    stats.to_csv(path)
    restored_stats = Statistics.from_csv(path)
    assert_allclose(restored_stats.data[0].best_cost, 0.92)
    assert_equal(restored_stats, stats)


@pytest.mark.parametrize("dtype", [np.float32, np.float64])
@pytest.mark.parametrize("transpose", [False, True])
def test_fractional_matrix_conversion(ok_small, dtype, transpose):
    """
    Matrix bindings preserve fractions and expose read-only float64 arrays.
    """
    matrix = ok_small.distance_matrix(0).astype(dtype) / 16
    if transpose:
        matrix = matrix.T
    data = ok_small.replace(
        distance_matrices=[matrix], duration_matrices=[matrix]
    )
    for result in (data.distance_matrix(0), data.duration_matrix(0)):
        assert_equal(result.dtype, np.float64)
        assert_(not result.flags.writeable)
        assert_allclose(result, matrix)
