from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import Model


def test_model_update():
    # Just after construction, the ProblemData instance has not yet been
    # constructed and should return None.
    model = Model()
    assert_(model.data is None)

    # Let's add some data: a single client, and edges from/to the depot.
    depot = model.add_depot(0, 0)
    client = model.add_client(0, 1, demand=1)
    model.add_edge(depot, client, 1, 1)
    model.add_edge(client, depot, 1, 1)
    model.add_vehicle_type(1, 1)

    # After calling update, a ProblemData instance is constructed. In this case
    # it should have one client.
    model.update()
    assert_(model.data is not None)
    assert_equal(model.data.num_clients, 1)  # type: ignore


def test_add_depot_raises_more_than_one_depot():
    model = Model()
    model.add_depot(0, 0)  # first depot should be OK

    with assert_raises(ValueError):
        model.add_depot(0, 1)  # second (and more) should not be


def test_add_edge_raises_negative_distance_or_duration():
    model = Model()
    depot = model.add_depot(0, 0)
    client = model.add_client(0, 1)

    model.add_edge(depot, client, distance=0, duration=0)  # zero should be OK

    with assert_raises(ValueError):  # negative distance should not be OK
        model.add_edge(client, depot, distance=-1, duration=0)

    with assert_raises(ValueError):  # negative duration should also not be OK
        model.add_edge(client, depot, distance=0, duration=-1)


def test_add_vehicle_type_raises_more_than_one_type():
    model = Model()
    model.add_vehicle_type(1, 10)  # first type should be OK

    with assert_raises(ValueError):
        model.add_vehicle_type(2, 10)  # second (and more) should not be


@mark.parametrize(
    "amount, capacity",
    [
        (0, 1),  # zero vehicles is not OK
        (-1, 1),  # negative vehicles is not OK
        (1, 0),  # zero capacity is not OK
        (1, -1),  # negative capacity is not OK
    ],
)
def test_add_vehicle_type_raises_negative_amount_or_capacity(amount, capacity):
    model = Model()

    with assert_raises(ValueError):
        model.add_vehicle_type(amount, capacity)


def test_read_and_solve():
    pass
