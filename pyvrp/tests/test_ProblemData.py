from numpy.testing import assert_

from pyvrp import Client, ProblemData


def test_depot_is_first_client():
    """
    The ``depot()`` helper should return the first client, that is,
    ``client(0)``.
    """
    depot = Client(x=0, y=0)
    clients = [
        depot,
        Client(x=0, y=1),
    ]
    data = ProblemData(
        clients=clients,
        nb_vehicles=1,
        vehicle_cap=1,
        duration_matrix=[[0, 1], [1, 0]],
    )

    assert_(data.depot() is data.client(0))
    # Note that the constructor makes a copy of the objects
    assert_(data.depot() is not depot)
