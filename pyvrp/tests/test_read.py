import ctypes

from numpy.testing import assert_equal, assert_raises
from pytest import mark

from pyvrp.tests.helpers import read


@mark.parametrize(
    "where, exception",
    [
        ("data/UnknownEdgeWeightFmt.txt", RuntimeError),
        ("data/UnknownEdgeWeightType.txt", RuntimeError),
        ("somewhere that does not exist", ValueError),
        ("data/FileWithUnknownSection.txt", RuntimeError),
        ("data/DepotNotOne.txt", RuntimeError),
        ("data/DepotSectionDoesNotEndInMinusOne.txt", RuntimeError),
        ("data/MoreThanOneDepot.txt", RuntimeError),
        ("data/NonZeroDepotServiceDuration.txt", RuntimeError),
        ("data/NonZeroDepotReleaseTime.txt", RuntimeError),
        ("data/NonZeroDepotOpenTimeWindow.txt", RuntimeError),
        ("data/NonZeroDepotDemand.txt", RuntimeError),
        ("data/TimeWindowOpenEqualToClose.txt", RuntimeError),
        ("data/TimeWindowOpenLargerThanClose.txt", RuntimeError),
        ("data/EdgeWeightsNoExplicit.txt", RuntimeError),
        ("data/EdgeWeightsNotFullMatrix.txt", RuntimeError),
    ],
)
def test_raises_invalid_file(where: str, exception: Exception):
    with assert_raises(exception):
        read(where)


def test_reading_OkSmall_instance():
    data = read("data/OkSmall.txt")

    # From the DIMENSION, VEHICLES, and CAPACITY fields in the file.
    assert_equal(data.num_clients, 4)
    assert_equal(data.num_vehicles, 3)
    assert_equal(data.vehicle_capacity, 10)

    # From the NODE_COORD_SECTION in the file
    expected = [
        (2334, 726),
        (226, 1297),
        (590, 530),
        (435, 718),
        (1191, 639),
    ]

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).x, expected[client][0])
        assert_equal(data.client(client).y, expected[client][1])

    # From the EDGE_WEIGHT_SECTION in the file
    expected = [
        [0, 1544, 1944, 1931, 1476],
        [1726, 0, 1992, 1427, 1593],
        [1965, 1975, 0, 621, 1090],
        [2063, 1433, 647, 0, 818],
        [1475, 1594, 1090, 828, 0],
    ]

    dist_mat = data.distance_matrix()

    for frm in range(data.num_clients + 1):  # incl. depot
        for to in range(data.num_clients + 1):  # incl. depot
            assert_equal(data.dist(frm, to), expected[frm][to])
            assert_equal(dist_mat[frm, to], expected[frm][to])

    # From the DEMAND_SECTION in the file
    expected = [0, 5, 5, 3, 5]

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).demand, expected[client])

    # From the TIME_WINDOW_SECTION in the file
    expected = [
        (0, 45000),
        (15600, 22500),
        (12000, 19500),
        (8400, 15300),
        (12000, 19500),
    ]

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).tw_early, expected[client][0])
        assert_equal(data.client(client).tw_late, expected[client][1])

    # From the SERVICE_TIME_SECTION in the file
    expected = [0, 360, 360, 420, 360]

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).serv_dur, expected[client])


def test_reading_En22k4_instance():  # instance from CVRPLIB
    data = read("data/E-n22-k4.vrp.txt")

    assert_equal(data.num_clients, 21)
    assert_equal(data.vehicle_capacity, 6_000)

    assert_equal(data.depot().x, 145)  # depot [x, y] location
    assert_equal(data.depot().y, 215)

    assert_equal(data.client(1).x, 151)  # first customer [x, y] location
    assert_equal(data.client(1).y, 264)

    # The data file specifies distances as 2D Euclidean. We take that and
    # should compute integer equivalents with up to one decimal precision.
    # For depot -> first customer:
    # For depot -> first customer:
    #      dX = 151 - 145 = 6
    #      dY = 264 - 215 = 49
    #      dist = sqrt(dX^2 + dY^2) = 49.37
    #      int(10 * dist) = 493
    assert_equal(data.dist(0, 1), 493)
    assert_equal(data.dist(1, 0), 493)

    # These fields are not present in the data file, and should thus retain
    # their default values.
    max_int = 2 ** (8 * ctypes.sizeof(ctypes.c_int) - 1) - 1

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).serv_dur, 0)
        assert_equal(data.client(client).tw_early, 0)
        assert_equal(data.client(client).tw_late, max_int)
        assert_equal(data.client(client).release_time, 0)
