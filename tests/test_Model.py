import numpy as np
import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    Client,
    ClientGroup,
    Depot,
    Location,
    Model,
    PenaltyParams,
    Profile,
    SolveParams,
    VehicleType,
)
from pyvrp.constants import MAX_VALUE
from pyvrp.exceptions import ScalingWarning
from pyvrp.stop import MaxIterations
from tests.helpers import read_solution


def test_model_data():
    """
    Tests that calling the ``data()`` member on the model returns a data
    instance representing the added data.
    """
    model = Model()
    model.add_vehicle_type(capacity=1, num_available=1)

    # Let's add some data: a single client, and edges from/to the depot.
    depot_loc = model.add_location(0, 0)
    model.add_depot(location=depot_loc)

    client_loc = model.add_location(0, 1)
    model.add_client(location=client_loc, delivery=1)

    model.add_edge(depot_loc, client_loc, 1, 1)
    model.add_edge(client_loc, depot_loc, 1, 1)

    data = model.data()
    assert_equal(data.num_clients, 1)
    assert_equal(data.num_vehicle_types, 1)
    assert_equal(data.num_vehicles, 1)


def test_add_edge_raises_negative_distance_or_duration():
    """
    Negative distances or durations are not understood. Attempting to add
    such edges should raise an error.
    """
    model = Model()
    loc1 = model.add_location(0, 0)
    loc2 = model.add_location(0, 1)

    model.add_edge(loc1, loc2, distance=0, duration=0)  # zero should be OK

    with assert_raises(ValueError):  # negative distance should not be OK
        model.add_edge(loc2, loc1, distance=-1, duration=0)

    with assert_raises(ValueError):  # negative duration should also not be OK
        model.add_edge(loc2, loc1, distance=0, duration=-1)


def test_add_edge_raises_self_connection_nonzero_distance_or_duration():
    """
    Edges connecting a node to itself must have 0 distance and duration.
    """
    model = Model()
    loc1 = model.add_location(0, 0)
    loc2 = model.add_location(0, 1)

    model.add_edge(loc1, loc2, distance=1, duration=1)  # should be OK
    model.add_edge(loc1, loc1, distance=0, duration=0)  # should be OK
    model.add_edge(loc2, loc2, distance=0, duration=0)  # should be OK

    with assert_raises(ValueError):  # self loop with nonzero distance not OK
        model.add_edge(loc1, loc1, distance=1, duration=0)

    with assert_raises(ValueError):  # self loop with nonzero duration not OK
        model.add_edge(loc1, loc1, distance=0, duration=1)

    with assert_raises(ValueError):  # self loop with nonzero distance not OK
        model.add_edge(loc2, loc2, distance=1, duration=0)

    with assert_raises(ValueError):  # self loop with nonzero duration not OK
        model.add_edge(loc2, loc2, distance=0, duration=1)


def test_add_client_attributes():
    """
    Smoke test that checks, for a single client, that the model adds a client
    whose attributes are the same as what was passed in.
    """
    model = Model()
    client = model.add_client(
        location=model.add_location(1, 2),
        delivery=3,
        pickup=9,
        service_duration=4,
        tw_early=5,
        tw_late=6,
        release_time=0,
        prize=8,
        required=False,
    )

    assert_equal(client.location, 0)
    assert_equal(client.delivery, [3])
    assert_equal(client.pickup, [9])
    assert_equal(client.service_duration, 4)
    assert_equal(client.tw_early, 5)
    assert_equal(client.tw_late, 6)
    assert_equal(client.release_time, 0)
    assert_equal(client.prize, 8)
    assert_(not client.required)


def test_add_client_with_multidimensional_load():
    """
    Smoke test that checks if multidimensional load is set correctly.
    """
    model = Model()
    loc = model.add_location(x=1, y=2)
    client = model.add_client(loc, delivery=[3, 4], pickup=[5, 6])

    assert_equal(client.delivery, [3, 4])
    assert_equal(client.pickup, [5, 6])


def test_add_depot_attributes():
    """
    Smoke test that checks the depot attributes are the same as what was passed
    in.
    """
    model = Model()
    loc = model.add_location(x=1, y=0)
    depot = model.add_depot(loc, tw_early=5, tw_late=7)
    assert_equal(depot.location, 0)
    assert_equal(depot.tw_early, 5)
    assert_equal(depot.tw_late, 7)


def test_add_edge():
    """
    Smoke test that checks edge attributes are the same as what was passed in.
    """
    model = Model()
    loc1 = model.add_location(0, 0)
    loc2 = model.add_location(0, 1)
    edge = model.add_edge(loc1, loc2, distance=15, duration=49)

    assert_(edge.frm is loc1)
    assert_(edge.to is loc2)
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
        shift_duration=93,
        max_distance=97,
        start_late=18,
        max_overtime=43,
    )

    assert_equal(vehicle_type.num_available, 10)
    assert_equal(vehicle_type.capacity, [998])
    assert_equal(vehicle_type.fixed_cost, 1_001)
    assert_equal(vehicle_type.tw_early, 17)
    assert_equal(vehicle_type.start_late, 18)
    assert_equal(vehicle_type.tw_late, 19)
    assert_equal(vehicle_type.shift_duration, 93)
    assert_equal(vehicle_type.max_distance, 97)
    assert_equal(vehicle_type.max_overtime, 43)


def test_add_vehicle_type_default_depots():
    """
    Tests that ``Model.add_vehicle_type`` correctly sets the (default) depot
    attributes on the vehicle type.
    """
    m = Model()
    depot1 = m.add_depot(m.add_location(0, 0))
    depot2 = m.add_depot(m.add_location(1, 1))

    # No depot specified: should default to the first (depot index 0).
    vehicle_type1 = m.add_vehicle_type()
    assert_equal(vehicle_type1.start_depot, 0)
    assert_equal(vehicle_type1.end_depot, 0)

    # First depot specified, should set first (depot index 0).
    vehicle_type2 = m.add_vehicle_type(start_depot=depot1, end_depot=depot1)
    assert_equal(vehicle_type2.start_depot, 0)
    assert_equal(vehicle_type2.end_depot, 0)

    # Second depot specified, should set second (depot index 1).
    vehicle_type3 = m.add_vehicle_type(start_depot=depot2, end_depot=depot2)
    assert_equal(vehicle_type3.start_depot, 1)
    assert_equal(vehicle_type3.start_depot, 1)

    # A mix is also OK.
    vehicle_type3 = m.add_vehicle_type(start_depot=depot1, end_depot=depot2)
    assert_equal(vehicle_type3.start_depot, 0)
    assert_equal(vehicle_type3.end_depot, 1)


def test_add_vehicle_type_raises_for_unknown_depot():
    """
    Tests that adding a vehicle type with a depot that's not known to the model
    raises a ValueError.
    """
    m = Model()
    depot = Depot(location=0)

    with assert_raises(ValueError):
        m.add_vehicle_type(start_depot=depot)

    with assert_raises(ValueError):
        m.add_vehicle_type(end_depot=depot)


def test_get_clients():
    """
    Tests the ``clients`` property.
    """
    model = Model()
    loc = model.add_location(0, 0)
    client1 = model.add_client(location=loc)
    client2 = model.add_client(location=loc)

    # Test that we can get the clients by index, or as a list.
    assert_equal(model.clients[0], client1)
    assert_equal(model.clients[1], client2)
    assert_equal(model.clients, [client1, client2])


def test_get_depots():
    """
    Tests the ``depots`` property.
    """
    model = Model()
    loc = model.add_location(0, 0)
    depot1 = model.add_depot(location=loc)
    depot2 = model.add_depot(location=loc)

    # Test that we can get the depots by index, or as a list.
    assert_equal(model.depots[0], depot1)
    assert_equal(model.depots[1], depot2)
    assert_equal(model.depots, [depot1, depot2])


def test_get_locations():
    """
    Checks that the ``locations`` property returns all locations.
    """
    model = Model()
    loc1 = model.add_location(0, 0)
    loc2 = model.add_location(0, 1)

    assert_equal(model.locations[0], loc1)
    assert_equal(model.locations[1], loc2)
    assert_equal(model.locations, [loc1, loc2])


def test_get_vehicle_types():
    """
    Tests the ``vehicle_types`` property.
    """
    model = Model()
    vehicle_type1 = model.add_vehicle_type(1, capacity=2)
    vehicle_type2 = model.add_vehicle_type(1, capacity=3)

    # Test that we can get the vehicle types by index, or as a list.
    assert_equal(model.vehicle_types[0], vehicle_type1)
    assert_equal(model.vehicle_types[1], vehicle_type2)
    assert_equal(model.vehicle_types, [vehicle_type1, vehicle_type2])


def test_from_data(small_cvrp):
    """
    Tests that initialising the model from a data instance results in a valid
    model representation of that data instance.
    """
    m = Model.from_data(small_cvrp)
    m_data = m.data()

    # We can first check if the overall problem dimension numbers agree.
    assert_equal(m_data.num_clients, small_cvrp.num_clients)
    assert_equal(m_data.num_vehicles, small_cvrp.num_vehicles)
    assert_equal(
        m_data.vehicle_type(0).capacity,
        small_cvrp.vehicle_type(0).capacity,
    )

    assert_equal(m_data.distance_matrices(), small_cvrp.distance_matrices())
    assert_equal(m_data.duration_matrices(), small_cvrp.duration_matrices())


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
    model.add_vehicle_type(
        num_available=3,
        capacity=10,
        tw_early=0,
        tw_late=45000,
    )

    coords = [(2334, 726), (226, 1297), (590, 530), (435, 718), (1191, 639)]
    for x, y in coords:
        model.add_location(x, y)

    model.add_depot(location=model.locations[0])
    model.add_client(model.locations[1], 5, 0, 360, 15600, 22500)
    model.add_client(model.locations[2], 5, 0, 360, 12000, 19500)
    model.add_client(model.locations[3], 3, 0, 420, 8400, 15300)
    model.add_client(model.locations[4], 5, 0, 360, 12000, 19500)

    edge_weights = [
        [0, 1544, 1944, 1931, 1476],
        [1726, 0, 1992, 1427, 1593],
        [1965, 1975, 0, 621, 1090],
        [2063, 1433, 647, 0, 818],
        [1475, 1594, 1090, 828, 0],
    ]

    for idx_frm, frm in enumerate(model.locations):
        for idx_to, to in enumerate(model.locations):
            edge_weight = edge_weights[idx_frm][idx_to]
            model.add_edge(frm, to, edge_weight, edge_weight)

    res = model.solve(stop=MaxIterations(100), seed=0)

    assert_(res.is_feasible())
    assert_equal(res.cost(), 9_155)


def test_model_solve_display_argument(ok_small, caplog):
    """
    Tests that solving a model displays solver progress when the ``display``
    argument is ``True``.
    """
    model = Model.from_data(ok_small)

    # First solve with display turned off. We should not see any output in this
    # case.
    model.solve(stop=MaxIterations(10), seed=0, display=False)
    printed = caplog.text
    assert_equal(printed, "")

    # Now solve with display turned on. We should see output now.
    res = model.solve(stop=MaxIterations(10), seed=0, display=True)
    printed = caplog.text

    # Check that some of the header data is in the output.
    assert_("PyVRP" in printed)
    assert_("Time" in printed)
    assert_("Iters" in printed)
    assert_("Current" in printed)
    assert_("Candidate" in printed)
    assert_("Best" in printed)

    # Check that we include the cost and total runtime in the output somewhere.
    assert_(str(round(res.cost())) in printed)
    assert_(str(round(res.runtime)) in printed)


@pytest.mark.parametrize("missing_value", [5, 100, MAX_VALUE, MAX_VALUE + 1])
def test_partial_distance_duration_matrix(missing_value):
    """
    Tests that adding a full distance or duration matrix is not required. Any
    "missing" arcs are given default values.
    """
    model = Model()
    model.add_vehicle_type()

    locs = [
        model.add_location(0, 0),
        model.add_location(0, 1),
        model.add_location(1, 0),
    ]

    model.add_depot(location=locs[0])
    model.add_client(location=locs[1])
    model.add_client(location=locs[2])

    # Only a subset of all possible edges exist. The model should be able to
    # handle that.
    model.add_edge(locs[0], locs[1], distance=1)
    model.add_edge(locs[1], locs[2], distance=2)
    model.add_edge(locs[2], locs[0], distance=1)

    # Edges that were not explicitly set should default to the missing value
    # argument, or MAX_VALUE, whichever is smaller.
    data = model.data(missing_value)
    distances = data.distance_matrix(profile=0)
    assert_equal(distances[0, 2], min(missing_value, MAX_VALUE))
    assert_equal(distances[1, 0], min(missing_value, MAX_VALUE))

    res = model.solve(MaxIterations(100), seed=4, missing_value=missing_value)
    assert_equal(res.best.num_routes(), 1)
    assert_equal(res.cost(), 4)  # depot -> client 1 -> client 2 -> depot
    assert_(res.is_feasible())


def test_data_warns_about_scaling_issues(recwarn):
    """
    Tests that the modelling interface warns when an edge is added whose
    distance or duration values are too large. This situation is likely caused
    by scaling issues, so a warning is appropriate.
    """
    model = Model()
    model.add_vehicle_type(capacity=0, num_available=1)
    loc1 = model.add_location(0, 0)
    loc2 = model.add_location(1, 1)

    # Normal distance sizes; should all be OK.
    for distance in [1, 10, 100, 1_000, 10_000, 100_000, MAX_VALUE]:
        model.add_edge(loc2, loc1, distance=distance)
        assert_equal(len(recwarn), 0)

    # But a value exceeding the maximum value is not OK. This should warn (both
    # for distance and/or duration).
    with pytest.warns(ScalingWarning):
        model.add_edge(loc1, loc2, distance=MAX_VALUE + 1)

    with pytest.warns(ScalingWarning):
        model.add_edge(loc1, loc2, distance=0, duration=MAX_VALUE + 1)


def test_model_solves_instance_with_zero_or_one_clients():
    """
    This test exercises the bug identified in issue #272, where the model
    could not solve an instance with zero clients or just one client.
    """
    m = Model()
    m.add_vehicle_type(num_available=1)

    depot_loc = m.add_location(0, 0)
    m.add_depot(location=depot_loc)

    # Solve an instance with no clients.
    res = m.solve(stop=MaxIterations(1))
    assert_equal(res.best.num_clients(), 0)

    # Solve an instance with one client.
    client_loc = m.add_location(0, 0)
    m.add_client(location=client_loc)
    m.add_edge(depot_loc, client_loc, distance=0)
    m.add_edge(client_loc, depot_loc, distance=0)

    res = m.solve(stop=MaxIterations(1))
    assert_equal(res.best.num_clients(), 1)


def test_model_solves_small_instance_with_fixed_costs():
    """
    High-level test that creates and solves a small instance with vehicle fixed
    costs, to see if the model (and thus the underlying solution algorithm)
    can handle that. This test exercises the bug identified in issue #380.
    Before fixing this bug, the solver would hang on this instance.
    """
    m = Model()

    for idx in range(2):
        m.add_vehicle_type(
            num_available=5,
            fixed_cost=10,
            tw_early=0,
            tw_late=40,
        )

    m.add_depot(m.add_location(0, 0))

    for idx in range(5):
        m.add_client(
            location=m.add_location(x=idx, y=idx),
            service_duration=1,
            tw_early=0,
            tw_late=20,
        )

    for idx_frm, frm in enumerate(m.locations):
        for idx_to, to in enumerate(m.locations):
            if idx_frm != idx_to:
                m.add_edge(frm, to, distance=0, duration=5)

    res = m.solve(stop=MaxIterations(10))
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
            num_available=2,
            tw_early=tw_early,
            tw_late=tw_late,
        )

    m.add_depot(location=m.add_location(0, 0))

    for idx in range(5):
        # Vehicles of the first type can visit a single client before having to
        # return to the depot. The second vehicle type can be used to visit two
        # clients before having to return to the depot. So we need at least
        # three routes.
        m.add_client(
            location=m.add_location(x=idx, y=idx),
            service_duration=1,
            tw_early=0,
            tw_late=20,
        )

    for idx_frm, frm in enumerate(m.locations):
        for idx_to, to in enumerate(m.locations):
            if idx_frm != idx_to:
                m.add_edge(frm, to, distance=0, duration=5)

    res = m.solve(stop=MaxIterations(100))
    assert_(res.is_feasible())
    assert_(res.best.num_routes() > 2)


def test_model_solves_line_instance_with_multiple_depots():
    """
    High-level test that creates and solves a small instance on a line, where
    there are two depots. This test checks that the model and underlying
    algorithm correctly handle multiple depots.
    """
    m = Model()

    depot1 = m.add_depot(location=m.add_location(0, 0))  # location 0
    depot2 = m.add_depot(location=m.add_location(5, 0))  # location 1

    m.add_vehicle_type(1, capacity=2, start_depot=depot1, end_depot=depot1)
    m.add_vehicle_type(1, capacity=2, start_depot=depot2, end_depot=depot2)

    for idx in range(1, 5):  # locations 2, 3, 4, and 5
        m.add_client(location=m.add_location(x=idx, y=0), delivery=1)

    # All locations are on a horizontal line, with the depots on each end. The
    # line is organised as follows:
    #     D0 C0 C1 C2 C3 D1   (depot or client)
    #      0  2  3  4  5  1   (location index)
    # Thus, C0 and C1 are closest to D0, and C2 and C3 are closest to D1.
    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, distance=abs(frm.x - to.x))

    res = m.solve(stop=MaxIterations(100), seed=3)
    assert_equal(res.cost(), 8)
    assert_(res.is_feasible())

    # Test that there are two routes, with the clients closest to depot 0
    # assigned to the first route, and clients closest to depot 1 assigned to
    # the second route. Route membership is compared using sets because the
    # optimal route order is not unique.
    route1, route2 = res.best.routes()
    clients1 = {activity.idx for activity in route1 if activity.is_client()}
    clients2 = {activity.idx for activity in route2 if activity.is_client()}
    assert_equal(clients1, {0, 1})
    assert_equal(clients2, {2, 3})


def test_client_depot_and_vehicle_type_name_fields():
    """
    Tests that name fields are properly passed to client, depot, and vehicle
    types.
    """
    m = Model()

    depot = m.add_depot(m.add_location(1, 1), name="depot1")
    assert_equal(depot.name, "depot1")
    assert_equal(str(depot), "depot1")

    veh_type = m.add_vehicle_type(name="veh_type1")
    assert_equal(veh_type.name, "veh_type1")
    assert_equal(str(veh_type), "veh_type1")

    client = m.add_client(m.add_location(1, 2), name="client1")
    assert_equal(client.name, "client1")
    assert_equal(str(client), "client1")


@pytest.mark.parametrize(
    ("pickups", "deliveries", "expected_excess_load"),
    [
        # The route should have 1 excess load (since the total pickup amount
        # sums to 11, and the vehicle capacity is 10). Same with similar
        # deliveries.
        ([1, 2, 3, 5], [0, 0, 0, 0], 1),
        ([0, 0, 0, 0], [1, 2, 3, 5], 1),
        ([1, 2, 3, 5], [1, 2, 3, 5], 1),
        # The following pickup and delivery schedule is tight, but should be
        # fine: the vehicle leaves full, and returns full, but there is a
        # configuration whereby it never exceeds its capacity along the way.
        ([1, 2, 3, 4], [4, 3, 2, 1], 0),
        # And no delivery or pickup amounts should of course also be OK!
        ([0, 0, 0, 0], [0, 0, 0, 0], 0),
    ],
)
def test_model_solves_instances_with_pickups_and_deliveries(
    pickups: list[int],
    deliveries: list[int],
    expected_excess_load: int,
):
    """
    High-level test that creates and solves a single-route instance where
    clients have pickups, deliveries, and sometimes both at the same time.
    """
    m = Model()
    m.add_vehicle_type(capacity=10)
    m.add_depot(location=m.add_location(0, 0))

    for idx in range(4):
        m.add_client(
            location=m.add_location(idx + 1, idx + 1),
            delivery=deliveries[idx],
            pickup=pickups[idx],
        )

    for frm in m.locations:
        for to in m.locations:
            manhattan = abs(frm.x - to.x) + abs(frm.y - to.y)
            m.add_edge(frm, to, distance=int(manhattan))

    res = m.solve(stop=MaxIterations(100))
    route = res.best.routes()[0]

    assert_equal(route.excess_load(), [expected_excess_load])
    assert_equal(route.has_excess_load(), expected_excess_load > 0)


def test_add_client_raises_unknown_group():
    """
    Tests that the model's ``add_client`` method raises when it's given a group
    argument that is not known to the model.
    """
    m = Model()
    loc = m.add_location(1, 1)
    group = ClientGroup()

    with assert_raises(ValueError):
        m.add_client(location=loc, group=group)


def test_from_data_client_group(ok_small):
    """
    Test that creating a model from a given data instance with client groups
    correctly sets up the client groups in the model.
    """
    clients = ok_small.clients()
    clients[0] = Client(1, delivery=[1], required=False, group=0)
    clients[1] = Client(2, delivery=[1], required=False, group=0)

    group = ClientGroup([0, 1])

    data = ok_small.replace(clients=clients, groups=[group])
    model = Model.from_data(data)

    # Test that the clients have been correctly registered, and that there is
    # a client group in the model.
    assert_equal(model.clients[0].group, 0)
    assert_equal(model.clients[1].group, 0)
    assert_equal(len(model.groups), 1)

    # Test that that group actually contains the clients.
    group = model.groups[0]
    assert_(group.required)
    assert_equal(len(group), 2)
    assert_equal(group.clients, [0, 1])


def test_to_data_client_group():
    """
    Tests that creating a small model with a client group results in a correct
    ProblemData instance that has the appropriate group memberships set up.
    """
    m = Model()
    m.add_vehicle_type()
    m.add_depot(location=m.add_location(1, 1))

    group = m.add_client_group()
    m.add_client(location=m.add_location(1, 1), required=False, group=group)
    m.add_client(location=m.add_location(2, 2), required=False, group=group)

    # Generate the data instance. There should be a single client group, and
    # the first two clients should be members of that group.
    data = m.data()
    assert_equal(data.num_groups, 1)

    group = data.group(0)
    assert_equal(group.clients, [0, 1])


def test_raises_mutually_exclusive_client_group_required_client():
    """
    Tests that adding a required client to a mutually exclusive client group
    raises, because that does not make any sense.
    """
    m = Model()
    loc = m.add_location(1, 1)

    group = m.add_client_group()
    assert_(group.mutually_exclusive)

    with assert_raises(ValueError):
        m.add_client(location=loc, required=True, group=group)


def test_tsp_instance_with_mutually_exclusive_groups(gtsp):
    """
    Smoke test that tests if the model can solve a generalised TSP instance
    where all clients are spread over fifty mutually exclusive groups.
    """
    m = Model.from_data(gtsp)
    res = m.solve(stop=MaxIterations(5))

    assert_(res.best.is_feasible())
    assert_equal(res.best.num_clients(), gtsp.num_groups)
    assert_equal(res.best.num_missing_groups(), 0)


def test_minimise_distance_or_duration(ok_small):
    """
    Small test that checks the model knows how to solve instances with
    different objective values.
    """
    orig_model = Model.from_data(ok_small)

    vehicle_types = [
        VehicleType(capacity=[10], unit_distance_cost=1, unit_duration_cost=0),
        VehicleType(capacity=[10], unit_distance_cost=0, unit_duration_cost=1),
    ]
    data = ok_small.replace(vehicle_types=vehicle_types)
    new_model = Model.from_data(data)

    orig_res = orig_model.solve(stop=MaxIterations(20), seed=82)
    new_res = new_model.solve(stop=MaxIterations(20), seed=82)

    assert_equal(orig_res.cost(), 9_155)
    assert_equal(new_res.cost(), 9_875)

    # The given instance has the same distance and duration matrix. There is
    # thus no difference in actual travel time or distance. But the duration
    # objective should also count service duration along the route, and that
    # is something we can check.
    service = sum(data.client(client).service_duration for client in [0, 3])
    assert_equal(new_res.cost(), orig_res.cost() + service)


def test_adding_vehicle_type_with_unknown_profile_raises():
    """
    Tests that adding a vehicle type with a routing profile that is not in the
    model raises.
    """
    m = Model()

    profile = Profile()
    assert_(profile not in m.profiles)

    with assert_raises(ValueError):
        m.add_vehicle_type(profile=profile)


def test_adding_multiple_routing_profiles():
    """
    Tests that adding multiple routing profiles to the model works, and the
    solver finds the optimal solution.
    """
    m = Model()

    profile1 = m.add_profile()
    assert_(m.profiles[0] is profile1)

    profile2 = m.add_profile()
    assert_(m.profiles[1] is profile2)

    veh_type1 = m.add_vehicle_type(profile=profile1)
    assert_equal(veh_type1.profile, 0)

    veh_type2 = m.add_vehicle_type(profile=profile2)
    assert_equal(veh_type2.profile, 1)

    m.add_depot(location=m.add_location(1, 1))
    m.add_client(location=m.add_location(2, 2))

    for idx_frm, frm in enumerate(m.locations):
        for idx_to, to in enumerate(m.locations):
            if idx_frm != idx_to:
                m.add_edge(frm, to, distance=10, duration=5, profile=profile1)
                m.add_edge(frm, to, distance=5, duration=10, profile=profile2)

    data = m.data()
    assert_equal(data.num_profiles, 2)

    # Check that the distance and duration matrices of both profiles are
    # defined correctly.
    assert_equal(data.distance_matrix(profile=0), [[0, 10], [10, 0]])
    assert_equal(data.duration_matrix(profile=0), [[0, 5], [5, 0]])
    assert_equal(data.distance_matrix(profile=1), [[0, 5], [5, 0]])
    assert_equal(data.duration_matrix(profile=1), [[0, 10], [10, 0]])

    res = m.solve(stop=MaxIterations(10))
    assert_(res.is_feasible())
    assert_equal(res.cost(), 10)


def test_profiles_build_on_base_edges():
    """
    Tests that distance and duration matrices belonging to different profiles
    are all constructed from the same base matrices, with profile-specific
    changes applied on top.
    """
    m = Model()
    m.add_vehicle_type()

    depot_loc = m.add_location(1, 1)
    m.add_depot(depot_loc)

    client_loc = m.add_location(2, 2)
    m.add_client(client_loc)

    # Add base edges. These edges are used to construct base matrices that the
    # profiles build on. Essentially, if an edge is not specifically provided
    # in the profile, it will be inherited from the base edges.
    for frm in m.locations:
        for to in m.locations:
            dist = abs(frm.x - to.x) + abs(frm.y - to.y)
            m.add_edge(frm, to, dist)

    prof1 = m.add_profile()
    prof2 = m.add_profile()

    # We have not yet added profile-specific edges. This means the profile
    # matrices should all be the same as the base matrices.
    data = m.data()
    assert_equal(data.distance_matrix(0), [[0, 2], [2, 0]])
    assert_equal(data.distance_matrix(1), [[0, 2], [2, 0]])
    assert_equal(data.duration_matrix(0), np.zeros((2, 2)))
    assert_equal(data.duration_matrix(1), np.zeros((2, 2)))

    # Let's now add a few profile-specific edges and test that these overwrite
    # the base data in the new data instance.
    m.add_edge(depot_loc, client_loc, distance=5, duration=10, profile=prof1)
    m.add_edge(depot_loc, client_loc, distance=10, duration=5, profile=prof2)

    data = m.data()
    assert_equal(data.distance_matrix(0), [[0, 5], [2, 0]])
    assert_equal(data.distance_matrix(1), [[0, 10], [2, 0]])
    assert_equal(data.duration_matrix(0), [[0, 10], [0, 0]])
    assert_equal(data.duration_matrix(1), [[0, 5], [0, 0]])


def test_model_solves_instances_with_multiple_profiles():
    """
    Smoke test to check that the model knows how to solve an instance with
    multiple profiles.
    """
    m = Model()
    m.add_depot(location=m.add_location(1, 1))

    m.add_client(location=m.add_location(1, 2))
    m.add_client(location=m.add_location(2, 1))

    prof1 = m.add_profile()  # this profile only cares about distance on x axis
    prof2 = m.add_profile()  # this one only about distance on y axis

    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, abs(frm.x - to.x), profile=prof1)
            m.add_edge(frm, to, abs(frm.y - to.y), profile=prof2)

    m.add_vehicle_type(1, profile=prof1)
    m.add_vehicle_type(1, profile=prof2)

    # The best we can do is have the first vehicle visit the first client (no
    # distance), and the second vehicle the second client (also no distance).
    # The resulting cost is thus zero.
    res = m.solve(stop=MaxIterations(10), seed=1)
    assert_equal(res.cost(), 0)

    route1, route2 = res.best.routes()
    assert_equal(str(route1), "C0")
    assert_equal(str(route2), "C1")


def test_model_solves_instance_with_zero_load_dimensions():
    """
    Smoke test to check that the model can solve an instance with zero load
    dimensions.
    """
    m = Model()
    m.add_depot(location=m.add_location(1, 1))

    m.add_client(location=m.add_location(1, 2))
    m.add_client(location=m.add_location(2, 1))
    m.add_client(location=m.add_location(2, 2))

    for frm in m.locations:
        for to in m.locations:
            manhattan = abs(frm.x - to.x) + abs(frm.y - to.y)
            m.add_edge(frm, to, distance=manhattan)

    m.add_vehicle_type(1)

    assert_equal(m.data().num_load_dimensions, 0)

    res = m.solve(stop=MaxIterations(10))

    assert_(res.is_feasible())
    assert_equal(res.best.num_routes(), 1)

    # The best we can do is to first visit client 1 or 3, then visit client 2,
    # then the remaining client (1 or 3), and finally return to the depot. This
    # results in a distance of 1 + 1 + 1 + 1 = 4.
    route = res.best.routes()[0]
    assert_equal(route.distance(), 4)


def test_bug_client_group_indices():
    """
    Tests the bug of #681. Because empty client groups compare equal, the
    group to index implementation returned the first object that compared
    equal, in this case resulting in both clients being inserted into the
    first client group rather than the each into one.
    """
    m = Model()
    loc = m.add_location(0, 0)
    m.add_depot(location=loc)

    group1 = m.add_client_group()
    group2 = m.add_client_group()

    client1 = m.add_client(location=loc, required=False, group=group2)
    assert_equal(client1.group, 1)

    client2 = m.add_client(location=loc, required=False, group=group1)
    assert_equal(client2.group, 0)

    assert_equal(len(group1), 1)
    assert_equal(len(group2), 1)


def test_integer_vehicle_capacity_and_load_arguments_are_promoted_to_lists():
    """
    Tests that passing an integer capacity or initial load functions the same
    way as passing a list of a single integer - the integer arguments are
    automatically promoted to lists of integers.
    """
    m = Model()

    veh1 = m.add_vehicle_type(capacity=10, initial_load=1)
    assert_equal(veh1.capacity, [10])
    assert_equal(veh1.initial_load, [1])

    veh2 = m.add_vehicle_type(capacity=[10], initial_load=[1])
    assert_(veh1 == veh2)
    assert_equal(veh2.capacity, [10])
    assert_equal(veh2.initial_load, [1])


def test_adding_vehicle_reload_depots():
    """
    Smoke test that checks adding reload depots to the vehicle type works
    correctly.
    """
    m = Model()
    depot1 = m.add_depot(location=m.add_location(0, 0))
    depot2 = m.add_depot(location=m.add_location(1, 1))

    veh_type1 = m.add_vehicle_type(reload_depots=[depot1])
    assert_equal(veh_type1.reload_depots, [0])

    veh_type2 = m.add_vehicle_type(reload_depots=[depot1, depot2])
    assert_equal(veh_type2.reload_depots, [0, 1])


def test_adding_unknown_reload_depots_raises():
    """
    Tests that passing an unknown reload depot when creating a new vehicle
    type raises.
    """
    m = Model()
    depot = Depot(location=0)  # not in model

    with assert_raises(ValueError):
        m.add_vehicle_type(reload_depots=[depot])


def test_model_solves_multi_trip_instance():
    """
    Smoke test to check that the model can solve an instance with multiple
    trips / reloading.
    """
    m = Model()
    depot1 = m.add_depot(location=m.add_location(0, 0))
    depot2 = m.add_depot(location=m.add_location(0, 0))

    m.add_vehicle_type(capacity=[5], reload_depots=[depot1, depot2])

    for idx in range(3):  # all locations are on a horizontal line
        m.add_client(location=m.add_location(idx, 0), delivery=[5])

    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, distance=abs(frm.x - to.x))

    res = m.solve(stop=MaxIterations(10))
    assert_(res.is_feasible())
    assert_equal(res.cost(), 6)

    routes = res.best.routes()
    assert_equal(len(routes), 1)

    # This route transports the full 15 client delivery demand using a vehicle
    # with capacity of just 5 because it reloads twice along the route.
    route = routes[0]
    assert_equal(route.excess_load(), [0])
    assert_equal(route.delivery(), [15])
    assert_equal(route.num_trips(), 3)


def test_instance_with_multi_trip_and_release_times(mtvrptw_release_times):
    """
    Smoke test that tests if the model can solve a multi-trip VRP instance
    with release times. The instance is due to [1]_.

    References
    ----------
    .. [1] Yu Yang (2023). An Exact Price-Cut-and-Enumerate Method for the
           Capacitated Multitrip Vehicle Routing Problem with Time Windows.
           *Transportation Science* 57(1): 230-251.
           https://doi.org/10.1287/trsc.2022.1161.
    """
    m = Model.from_data(mtvrptw_release_times)
    params = SolveParams(penalty=PenaltyParams(min_penalty=100))
    res = m.solve(stop=MaxIterations(50), params=params)
    assert_(res.is_feasible())

    opt = read_solution("data/C201R0.25.sol", mtvrptw_release_times)
    assert_(opt.is_feasible())

    # A proven optimal solution to this instance has cost 10687. The following
    # is a smoke test to verify that we are not too far (>10%) away after a few
    # iterations.
    opt_cost = opt.distance_cost()
    assert_equal(opt_cost, 10687)
    assert_(res.cost() < 1.1 * opt_cost)


def test_profile_name_and_str():
    """
    Tests that passing a name argument for a routing profile works correctly.
    """
    profile = Profile()  # name should be optional, and default empty
    assert_equal(profile.name, "")

    profile = Profile(name="test1")  # name should be set correctly
    assert_equal(profile.name, "test1")
    assert_equal(str(profile), "test1")

    m = Model()  # setting the name via the model should also work
    profile = m.add_profile(name="test2")
    assert_equal(profile.name, "test2")
    assert_equal(str(profile), "test2")


def test_model_solve_initial_solution(rc208):
    """
    Tests that the Model correctly forwards an initial solution.
    """
    m = Model.from_data(rc208)

    bks = read_solution("data/RC208.sol", rc208)
    res = m.solve(stop=MaxIterations(0), initial_solution=bks)
    assert_equal(res.best, bks)


def test_solution_satisfies_group_constraints():
    """
    Tests that the solution returned after solving satisfies group feasibility
    constraints.
    """
    m = Model()
    m.add_depot(location=m.add_location(0, 0))
    m.add_vehicle_type()

    group = m.add_client_group(required=False)
    for _ in range(100):
        m.add_client(
            location=m.add_location(0, 0),
            prize=1000,
            required=False,
            group=group,
        )

    for frm in m.locations:
        for to in m.locations:
            m.add_edge(frm, to, 0, 0)

    res = m.solve(stop=MaxIterations(1), seed=42)
    assert_equal(res.best.num_missing_groups(), 0)


def test_raises_unknown_location():
    """
    Tests that the model interface raises when given a location argument that
    it does not know about.
    """
    m = Model()
    loc = Location(0, 0)  # unknown, created outside of the model

    with assert_raises(ValueError):
        m.add_depot(location=loc)

    with assert_raises(ValueError):
        m.add_client(location=loc)

    # Now created via the model interface. This should be OK.
    loc = m.add_location(0, 0)
    m.add_depot(location=loc)
    m.add_client(location=loc)


def test_adding_location():
    """
    Tests adding and using a location.
    """
    m = Model()

    # Add location and test the attributes.
    loc = m.add_location(x=0.0, y=1.2, name="test")
    assert_equal(loc.x, 0.0)
    assert_equal(loc.y, 1.2)
    assert_equal(loc.name, "test")

    # Add a client at the location. Since the location is the first one, the
    # client should have a location index of 0.
    client = m.add_client(location=loc)
    assert_equal(client.location, 0)

    # Add another location and client. The client should have a location index
    # of 1.
    loc2 = m.add_location(x=1, y=1)
    client2 = m.add_client(location=loc2)
    assert_equal(client2.location, 1)


def test_solve_clients_in_same_location():
    """
    Smoke test that checks if the model can solve an instance with clients in
    the same location.
    """
    m = Model()
    m.add_vehicle_type()

    # Two clients in the same place.
    loc = m.add_location(10, 10)
    client1 = m.add_client(loc)
    client2 = m.add_client(loc)
    assert_equal(client1.location, 0)
    assert_equal(client2.location, 0)

    depot = m.add_depot(location=m.add_location(5, 5))
    assert_equal(depot.location, 1)

    for frm in m.locations:
        for to in m.locations:
            dist = abs(frm.x - to.x) + abs(frm.y - to.y)
            m.add_edge(frm, to, dist)

    res = m.solve(stop=MaxIterations(10))

    best = res.best
    assert_(best.is_feasible())
    assert_equal(best.num_clients(), 2)
    assert_equal(best.distance(), 20)  # depot loc to clients loc, and back
