from numpy.testing import (
    assert_,
    assert_allclose,
    assert_equal,
    assert_raises,
    assert_warns,
)

from pyvrp import Model
from pyvrp.constants import MAX_USER_VALUE, MAX_VALUE
from pyvrp.exceptions import EmptySolutionWarning, ScalingWarning
from pyvrp.stop import MaxIterations


def test_model_data():
    """
    Tests that calling the ``data()`` member on the model returns a data
    instance representing the added data.
    """
    model = Model()

    # Let's add some data: a single client, and edges from/to the depot.
    depot = model.add_depot(0, 0)
    client = model.add_client(0, 1, demand=1)
    model.add_edge(depot, client, 1, 1)
    model.add_edge(client, depot, 1, 1)
    model.add_vehicle_type(capacity=1, num_available=1)

    data = model.data()
    assert_equal(data.num_clients, 1)
    assert_equal(data.num_vehicle_types, 1)
    assert_equal(data.num_vehicles, 1)


def test_add_depot_raises_more_than_one_depot():
    """
    PyVRP does not currently support VRPs with multiple depots. Adding more
    than one depot should raise.
    """
    model = Model()
    model.add_depot(0, 0)  # first depot should be OK

    with assert_raises(ValueError):
        model.add_depot(0, 1)  # second (and more) should not be


def test_add_edge_raises_negative_distance_or_duration():
    """
    Negative distances or durations are not understood. Attempting to add
    such edges should raise an error.
    """
    model = Model()
    depot = model.add_depot(0, 0)
    client = model.add_client(0, 1)

    model.add_edge(depot, client, distance=0, duration=0)  # zero should be OK

    with assert_raises(ValueError):  # negative distance should not be OK
        model.add_edge(client, depot, distance=-1, duration=0)

    with assert_raises(ValueError):  # negative duration should also not be OK
        model.add_edge(client, depot, distance=0, duration=-1)


def test_add_client_attributes():
    """
    Smoke test that checks, for a single client, that the model adds a client
    whose attributes are the same as what was passed in.
    """
    model = Model()
    client = model.add_client(
        x=1,
        y=2,
        demand=3,
        service_duration=4,
        tw_early=5,
        tw_late=6,
        release_time=7,
        prize=8,
        required=False,
    )

    assert_equal(client.x, 1)
    assert_equal(client.y, 2)
    assert_equal(client.demand, 3)
    assert_equal(client.service_duration, 4)
    assert_equal(client.tw_early, 5)
    assert_equal(client.tw_late, 6)
    assert_equal(client.release_time, 7)
    assert_equal(client.prize, 8)
    assert_(not client.required)


def test_add_depot_attributes():
    """
    Smoke test that checks the depot attributes are the same as what was passed
    in.
    """
    model = Model()
    depot = model.add_depot(x=1, y=0, tw_early=3, tw_late=5)

    assert_equal(depot.x, 1)
    assert_equal(depot.y, 0)
    assert_equal(depot.tw_early, 3)
    assert_equal(depot.tw_late, 5)


def test_add_edge():
    """
    Smoke test that checks edge attributes are the same as what was passed in.
    """
    model = Model()
    depot = model.add_depot(0, 0)
    client = model.add_client(0, 1)
    edge = model.add_edge(depot, client, distance=15, duration=49)

    assert_(edge.frm is depot)
    assert_(edge.to is client)
    assert_equal(edge.distance, 15)
    assert_equal(edge.duration, 49)


def test_add_vehicle_type():
    """
    Smoke test that checks vehicle type attributes are the same as what was
    passed in.
    """
    model = Model()
    vehicle_type = model.add_vehicle_type(
        num_available=10,
        capacity=998,
        fixed_cost=1_001,
        tw_early=17,
        tw_late=19,
    )

    assert_equal(vehicle_type.num_available, 10)
    assert_allclose(vehicle_type.capacity, 998)
    assert_allclose(vehicle_type.fixed_cost, 1_001)
    assert_allclose(vehicle_type.tw_early, 17)
    assert_allclose(vehicle_type.tw_late, 19)


def test_get_locations():
    """
    Checks that the ``locations`` property returns the depot and all clients.
    """
    model = Model()
    client1 = model.add_client(0, 1)
    depot = model.add_depot(0, 0)
    client2 = model.add_client(0, 2)

    # Test that depot is always first and that we can get the clients by index.
    assert_equal(model.locations[0], depot)
    assert_equal(model.locations[1], client1)
    assert_equal(model.locations[2], client2)


def test_get_vehicle_types():
    """
    Tests the ``vehicle_types`` property.
    """
    model = Model()
    vehicle_type1 = model.add_vehicle_type(1, 2)
    vehicle_type2 = model.add_vehicle_type(1, 3)

    # Test that we can get the vehicle types by index.
    assert_equal(model.vehicle_types[0], vehicle_type1)
    assert_equal(model.vehicle_types[1], vehicle_type2)


def test_from_data(small_cvrp):
    """
    Tests that initialising the model from a data instance results in a valid
    model representation of that data instance.
    """
    model = Model.from_data(small_cvrp)
    model_data = model.data()

    # We can first check if the overall problem dimension numbers agree.
    assert_equal(model_data.num_clients, small_cvrp.num_clients)
    assert_equal(model_data.num_vehicles, small_cvrp.num_vehicles)
    assert_equal(
        model_data.vehicle_type(0).capacity,
        small_cvrp.vehicle_type(0).capacity,
    )

    # It's a bit cumbersome to compare the whole matrices, so we use a few
    # sample traces from the distance and duration matrices instead.
    assert_equal(model_data.dist(3, 4), small_cvrp.dist(3, 4))
    assert_equal(model_data.duration(2, 1), small_cvrp.duration(2, 1))


def test_from_data_and_solve(small_cvrp, ok_small):
    """
    Tests that solving a model initialised from a data instance finds the
    correct (known) solutions.
    """
    model = Model.from_data(small_cvrp)
    res = model.solve(stop=MaxIterations(100), seed=0)
    assert_equal(res.cost(), 3_743)
    assert_(res.is_feasible())

    model = Model.from_data(ok_small)
    res = model.solve(stop=MaxIterations(100), seed=0)
    assert_equal(res.cost(), 9_155)
    assert_(res.is_feasible())


def test_model_and_solve(ok_small):
    """
    Tests that solving a model initialised using the modelling interface
    finds the correct (known) solutions.
    """
    model = Model.from_data(ok_small)
    res = model.solve(stop=MaxIterations(100), seed=0)
    assert_equal(res.cost(), 9_155)
    assert_(res.is_feasible())

    # Now do the same thing, but model the instance using the modelling API.
    # This should of course result in the same solution.
    model = Model()
    model.add_vehicle_type(capacity=10, num_available=3)
    depot = model.add_depot(x=2334, y=726, tw_early=0, tw_late=45000)
    clients = [
        model.add_client(226, 1297, 5, 360, 15600, 22500),
        model.add_client(590, 530, 5, 360, 12000, 19500),
        model.add_client(435, 718, 3, 420, 8400, 15300),
        model.add_client(1191, 639, 5, 360, 12000, 19500),
    ]

    edge_weights = [
        [0, 1544, 1944, 1931, 1476],
        [1726, 0, 1992, 1427, 1593],
        [1965, 1975, 0, 621, 1090],
        [2063, 1433, 647, 0, 818],
        [1475, 1594, 1090, 828, 0],
    ]

    for idx, client in enumerate(clients, 1):
        from_depot = edge_weights[0][idx]
        to_depot = edge_weights[idx][0]

        model.add_edge(depot, client, from_depot, from_depot)
        model.add_edge(client, depot, to_depot, to_depot)

        for other_idx, other in enumerate(clients, 1):
            from_client = edge_weights[idx][other_idx]
            to_client = edge_weights[other_idx][idx]

            model.add_edge(client, other, from_client, from_client)
            model.add_edge(other, client, to_client, to_client)

    res = model.solve(stop=MaxIterations(100), seed=0)

    assert_(res.is_feasible())
    assert_equal(res.cost(), 9_155)


def test_partial_distance_duration_matrix():
    """
    Tests that adding a full distance or duration matrix is not required. Any
    "missing" arcs are given large default values, ensuring they are never
    used.
    """
    model = Model()
    depot = model.add_depot(0, 0)
    clients = [model.add_client(0, 1), model.add_client(1, 0)]

    # Only a subset of all possible edges exist. The model should be able to
    # handle that.
    model.add_edge(depot, clients[0], distance=1)
    model.add_edge(clients[0], clients[1], distance=2)
    model.add_edge(clients[1], depot, distance=1)

    model.add_vehicle_type(capacity=0, num_available=1)

    # These edges were not set, so their distance values should default to the
    # maximum value we use for such edges.
    data = model.data()
    assert_equal(data.dist(0, 2), MAX_VALUE)
    assert_equal(data.dist(1, 0), MAX_VALUE)

    res = model.solve(stop=MaxIterations(100), seed=4)
    assert_equal(res.best.num_routes(), 1)
    assert_equal(res.cost(), 4)  # depot -> client 1 -> client 2 -> depot
    assert_(res.is_feasible())


def test_data_warns_about_scaling_issues(recwarn):
    """
    Tests that the modelling interface warns when an edge is added whose
    distance or duration values are too large: in that case, PyVRP might not be
    able to detect missing edges. This situation is likely caused by scaling
    issues, so a warning is appropriate.
    """
    model = Model()
    model.add_vehicle_type(capacity=0, num_available=1)
    depot = model.add_depot(0, 0)
    client = model.add_client(1, 1)

    # Normal distance sizes; should all be OK.
    for distance in [1, 10, 100, 1_000, 10_000, 100_000, MAX_USER_VALUE]:
        model.add_edge(client, depot, distance=distance)
        assert_equal(len(recwarn), 0)

    # But a value exceeding the maximum user value is not OK. This should warn
    # (both for distance and/or duration).
    with assert_warns(ScalingWarning):
        model.add_edge(depot, client, distance=MAX_USER_VALUE + 1)

    with assert_warns(ScalingWarning):
        model.add_edge(depot, client, distance=0, duration=MAX_USER_VALUE + 1)


def test_model_solves_instance_with_zero_or_one_clients():
    """
    This test exercises the bug identified in issue #272, where the model
    could not solve an instance with zero clients or just one client.
    """
    m = Model()
    m.add_vehicle_type(capacity=15, num_available=1)
    depot = m.add_depot(x=0, y=0)

    # Solve an instance with no clients.
    with assert_warns(EmptySolutionWarning):
        res = m.solve(stop=MaxIterations(1))

    solution = [r.visits() for r in res.best.get_routes()]
    assert_equal(solution, [])

    # Solve an instance with one client.
    clients = [m.add_client(x=0, y=0, demand=0)]
    m.add_edge(depot, clients[0], distance=0)
    m.add_edge(clients[0], depot, distance=0)

    res = m.solve(stop=MaxIterations(1))
    solution = [r.visits() for r in res.best.get_routes()]
    assert_equal(solution, [[1]])


def test_model_solves_small_instance_with_fixed_costs():
    """
    High-level test that creates and solves a small instance with vehicle fixed
    costs, to see if the model (and thus the underlying solution algorithm)
    can handle that. This test exercises the bug identified in issue #380.
    Before fixing this bug, the solver would hang on this instance.
    """
    m = Model()

    for idx in range(2):
        m.add_vehicle_type(capacity=0, num_available=5, fixed_cost=10)

    m.add_depot(x=0, y=0, tw_early=0, tw_late=40)

    for idx in range(5):
        m.add_client(x=idx, y=idx, service_duration=1, tw_early=0, tw_late=20)

    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, distance=0, duration=5)

    res = m.solve(stop=MaxIterations(100))
    assert_(res.is_feasible())


def test_model_solves_small_instance_with_shift_durations():
    """
    High-level test that creates and solves a small instance with shift
    durations, to see if the model (and thus the underlying solution algorithm)
    can handle that.
    """
    m = Model()

    # Two vehicle types with different shift time windows: the first has a
    # shift time window of [0, 15], the second of [5, 25]. There are four
    # vehicles in total, two for each vehicle type.
    for tw_early, tw_late in [(0, 15), (5, 25)]:
        m.add_vehicle_type(
            capacity=0,
            num_available=2,
            tw_early=tw_early,
            tw_late=tw_late,
        )

    m.add_depot(x=0, y=0, tw_early=0, tw_late=40)

    for idx in range(5):
        # Vehicles of the first type can visit two clients before having to
        # return to the depot. The second vehicle type can be used to visit
        # a single client before having to return to the depot. So we need
        # at least three routes.
        m.add_client(
            x=idx,
            y=idx,
            service_duration=1,
            tw_early=0,
            tw_late=20,
        )

    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, distance=0, duration=5)

    res = m.solve(stop=MaxIterations(100))
    assert_(res.is_feasible())
    assert_(res.best.num_routes() > 2)
