from numpy.testing import assert_, assert_equal

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


def test_matrix_access():
    """
    Tests that the distance_matrix() and duration_matrix() methods return the
    proper matrices for a small example.
    """
    data = ProblemData(
        coords=[(0, 0), (0, 1)],
        demands=[0, 0],
        nb_vehicles=1,
        vehicle_cap=1,
        time_windows=[(0, 10), (0, 10)],
        service_durations=[0, 0],
        duration_matrix=[[8, 2], [3, 17]],
    )

    dist_matrix = data.distance_matrix()
    dur_matrix = data.duration_matrix()

    # Tests if the distance matrix is correct.
    assert_equal(dist_matrix[0, 0], 8)
    assert_equal(dist_matrix[0, 1], 2)
    assert_equal(dist_matrix[1, 0], 3)
    assert_equal(dist_matrix[1, 1], 17)

    # Tests if the duration matrix equals the distance matrix (for now).
    assert_equal(dur_matrix[0, 0], dist_matrix[0, 0])
    assert_equal(dur_matrix[0, 1], dist_matrix[0, 1])
    assert_equal(dur_matrix[1, 0], dist_matrix[1, 0])
    assert_equal(dur_matrix[1, 1], dist_matrix[1, 1])
