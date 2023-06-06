from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Model


def test_data_is_none_after_init():
    # Just after construction, the ProblemData instance has not yet been
    # constructed and should return None.
    model = Model()
    assert_(model.data is None)

    # After calling update, a ProblemData instance is constructed. In this case
    # it should be empty.
    model.update()
    assert_(model.data is not None)
    assert_equal(model.data.num_clients, 0)  # type: ignore


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
