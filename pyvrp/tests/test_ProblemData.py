import numpy as np
from numpy.random import default_rng
from numpy.testing import assert_, assert_equal

from pyvrp import ProblemData


def test_depot_is_first_client():
    """
    The ``depot()`` helper should return the first client, that is,
    ``client(0)``.
    """
    mat = [[0, 1], [1, 0]]
    data = ProblemData(
        coords=[(0, 0), (0, 1)],
        demands=[0, 0],
        nb_vehicles=1,
        vehicle_cap=1,
        time_windows=[(0, 10), (0, 10)],
        service_durations=[0, 0],
        distance_matrix=mat,
        duration_matrix=mat,
    )

    assert_(data.depot() is data.client(0))


def test_matrix_access():
    """
    Tests that the ``duration()``, ``duration_matrix()``, ``dist()``, and
    ``distance_matrix()`` methods correctly index the underlying duration
    and distance matrices.
    """
    size = 6
    gen = default_rng(seed=42)
    dist_mat = gen.integers(500, size=(size, size))
    dur_mat = gen.integers(500, size=(size, size))

    assert_(not np.allclose(dist_mat, dur_mat))

    data = ProblemData(
        coords=[(0, 0)] * size,
        demands=[0] * size,
        nb_vehicles=1,
        vehicle_cap=1,
        time_windows=[(0, 10)] * size,
        service_durations=[0] * size,
        distance_matrix=dist_mat,  # type: ignore
        duration_matrix=dur_mat,  # type: ignore
    )

    dist_mat_data = data.distance_matrix()
    dur_mat_data = data.duration_matrix()

    for frm in range(size):
        for to in range(size):
            assert_equal(dur_mat[frm, to], data.duration(frm, to))
            assert_equal(dist_mat[frm, to], data.dist(frm, to))

            assert_equal(dur_mat_data[frm, to], dur_mat[frm, to])
            assert_equal(dist_mat_data[frm, to], dist_mat[frm, to])
