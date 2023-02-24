from numpy.testing import assert_

from pyvrp import ProblemData


def test_depot_is_first_client():
    """
    The ``depot()`` helper should return the first client, that is,
    ``client(0)``.
    """
    data = ProblemData(
        coords=[(0, 0), (0, 1)],
        demands=[0, 0],
        nb_vehicles=1,
        vehicle_cap=1,
        time_windows=[(0, 10), (0, 10)],
        service_durations=[0, 0],
        duration_matrix=[[0, 1], [1, 0]],
    )

    assert_(data.depot() is data.client(0))
