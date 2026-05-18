"""
Minimal test for elevation cost feature.

Creates a simple instance with elevation data and verifies that:
1. Elevation data is stored on Location and propagates through ProblemData
2. ProblemData.elevation_gain computes positive climb only
3. Routes track elevation cost weighted by VehicleType.unit_elevation_cost
4. Solutions aggregate elevation cost from routes
5. CostEvaluator includes elevation cost in penalised_cost
"""

from pyvrp import Model, Route, Solution
from pyvrp._pyvrp import CostEvaluator


def _build_data():
    """Build a 3-location instance: depot (0m), low (10m), high (100m)."""
    m = Model()

    depot_loc = m.add_location(x=0, y=0, elevation=0)
    low_loc = m.add_location(x=10, y=0, elevation=10, name="low")
    high_loc = m.add_location(x=0, y=10, elevation=100, name="high")

    depot = m.add_depot(depot_loc)
    client_low = m.add_client(low_loc, delivery=20, name="low")
    client_high = m.add_client(high_loc, delivery=30, name="high")

    profile = m.add_profile()
    for frm in (depot_loc, low_loc, high_loc):
        for to in (depot_loc, low_loc, high_loc):
            if frm is not to:
                m.add_edge(frm, to, distance=1000, duration=1000, profile=profile)

    m.add_vehicle_type(
        num_available=1, capacity=100, profile=profile, unit_elevation_cost=1
    )

    return m, depot, client_low, client_high


def test_location_elevation_stored():
    m, _, _, _ = _build_data()
    locs = m.locations
    assert locs[0].elevation == 0
    assert locs[1].elevation == 10
    assert locs[2].elevation == 100


def test_elevation_gain_positive_only():
    m, _, _, _ = _build_data()
    data = m.data()

    assert data.elevation_gain(0, 1) == 10  # depot -> low
    assert data.elevation_gain(0, 2) == 100  # depot -> high
    assert data.elevation_gain(1, 2) == 90  # low -> high
    assert data.elevation_gain(2, 1) == 0  # high -> low (downhill)
    assert data.elevation_gain(2, 0) == 0  # high -> depot (downhill)


def test_route_elevation_cost():
    m, _, _, _ = _build_data()
    data = m.data()

    # Route: depot -> low (20kg) -> high (30kg) -> depot.
    # Arcs (load × elevation_gain × unit_elevation_cost = 1):
    #   depot->low:  50 × 10 = 500
    #   low->high:   30 × 90 = 2700  (20 dropped at low, 30 still on board)
    #   high->depot:  0 × 0  = 0    (all dropped, and downhill anyway)
    route = Route(data, [0, 1], vehicle_type=0)
    assert route.elevation_cost() == 3200


def test_solution_aggregates_elevation_cost():
    m, _, _, _ = _build_data()
    data = m.data()

    route = Route(data, [0, 1], vehicle_type=0)
    solution = Solution(data, [route])

    assert solution.elevation_cost() == route.elevation_cost()


def test_penalised_cost_includes_elevation():
    m, _, _, _ = _build_data()
    data = m.data()

    route = Route(data, [0, 1], vehicle_type=0)
    solution = Solution(data, [route])

    evaluator = CostEvaluator(load_penalties=[0.0], tw_penalty=0.0, dist_penalty=0.0)
    expected = (
        solution.distance_cost()
        + solution.duration_cost()
        + solution.elevation_cost()
    )
    assert evaluator.penalised_cost(solution) == expected
