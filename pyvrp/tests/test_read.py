from math import sqrt

from numpy.testing import assert_equal, assert_raises, assert_warns
from pytest import mark

from pyvrp.exceptions import ScalingWarning
from pyvrp.tests.helpers import read


@mark.parametrize(
    ("where", "exception"),
    [
        ("data/UnknownEdgeWeightFmt.txt", ValueError),
        ("data/UnknownEdgeWeightType.txt", ValueError),
        ("somewhere that does not exist", FileNotFoundError),
        ("data/FileWithUnknownSection.txt", ValueError),
        ("data/DepotNotOne.txt", ValueError),
        ("data/MoreThanOneDepot.txt", ValueError),
        ("data/TimeWindowOpenLargerThanClose.txt", ValueError),
        ("data/EdgeWeightsNoExplicit.txt", ValueError),
        ("data/EdgeWeightsNotFullMatrix.txt", ValueError),
    ],
)
def test_raises_invalid_file(where: str, exception: Exception):
    """
    Tests that ``read()`` raises when there are issues with the given file.
    """
    with assert_raises(exception):
        read(where)


def test_raises_unknown_round_func():
    """
    Tests that ``read()`` raises when the rounding function is not known.
    """
    with assert_raises(TypeError):
        # Unknown round_func, so should raise.
        read("data/OkSmall.txt", round_func="asdbsadfas")

    # Is the default round_func, so should not raise.
    read("data/OkSmall.txt", round_func="none")


def test_reading_OkSmall_instance():
    """
    Tests that the parsed data from the "OkSmall" instance is correct.
    """
    data = read("data/OkSmall.txt")

    # From the DIMENSION, VEHICLES, and CAPACITY fields in the file.
    assert_equal(data.num_clients, 4)
    assert_equal(data.num_vehicles, 3)
    assert_equal(data.num_vehicle_types, 1)
    assert_equal(data.vehicle_type(0).capacity, 10)

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

    # For instances read through VRPLIB/read(), distance is duration. So the
    # dist/durs should be the same as the expected edge weights above.
    for frm in range(data.num_clients + 1):  # incl. depot
        for to in range(data.num_clients + 1):  # incl. depot
            assert_equal(data.dist(frm, to), expected[frm][to])
            assert_equal(data.duration(frm, to), expected[frm][to])

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
        assert_equal(data.client(client).service_duration, expected[client])


def test_reading_En22k4_instance():  # instance from CVRPLIB
    """
    Tests that the small E-n22-k4 instance from CVRPLIB is correctly parsed.
    """
    data = read("data/E-n22-k4.txt", round_func="trunc1")

    assert_equal(data.num_clients, 21)
    assert_equal(data.vehicle_type(0).capacity, 6_000)

    # Coordinates are scaled by 10 to align with 1 decimal distance precision
    assert_equal(data.client(0).x, 1450)  # depot [x, y] location
    assert_equal(data.client(0).y, 2150)

    assert_equal(data.client(1).x, 1510)  # first customer [x, y] location
    assert_equal(data.client(1).y, 2640)

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

    # This is a CVRP instance, so all other fields should have default values.
    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).service_duration, 0)
        assert_equal(data.client(client).tw_early, 0)
        assert_equal(data.client(client).tw_late, 0)
        assert_equal(data.client(client).release_time, 0)
        assert_equal(data.client(client).prize, 0)
        assert_equal(data.client(client).required, True)


def test_reading_RC208_instance():  # Solomon style instance
    """
    Tests that a Solomon-style VRPTW instance is correctly parsed.
    """
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )

    assert_equal(data.num_clients, 100)  # Excl. depot
    assert_equal(data.vehicle_type(0).capacity, 1_000)

    # Coordinates and times are scaled by 10 for 1 decimal distance precision
    assert_equal(data.client(0).x, 400)  # depot [x, y] location
    assert_equal(data.client(0).y, 500)
    assert_equal(data.client(0).tw_early, 0)
    assert_equal(data.client(0).tw_late, 9600)

    # Note: everything except demand is scaled by 10
    assert_equal(data.client(1).x, 250)  # first customer [x, y] location
    assert_equal(data.client(1).y, 850)
    assert_equal(data.client(1).demand, 20)
    assert_equal(data.client(1).tw_early, 3880)
    assert_equal(data.client(1).tw_late, 9110)
    assert_equal(data.client(1).service_duration, 100)

    # The data file specifies distances as 2D Euclidean. We take that and
    # should compute integer equivalents with up to one decimal precision.
    # For depot -> first customer:
    # For depot -> first customer:
    #      dX = 40 - 25 = 15
    #      dY = 50 - 85 = -35
    #      dist = sqrt(dX^2 + dY^2) = 38.07
    #      int(10 * dist) = 380
    assert_equal(data.dist(0, 1), 380)
    assert_equal(data.dist(1, 0), 380)

    for client in range(1, data.num_clients + 1):  # excl. depot
        assert_equal(data.client(client).service_duration, 100)

    # This is a VRPTW instance, so all other fields should have default values.
    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(data.client(client).release_time, 0)
        assert_equal(data.client(client).prize, 0)
        assert_equal(data.client(client).required, True)


def test_warns_about_scaling_issues():
    """
    Tests that ``read()`` warns about scaling issues when a distance value is
    very large.
    """
    with assert_warns(ScalingWarning):
        # The arc from the depot to client 4 is really large (one billion), so
        # that should trigger a warning.
        read("data/ReallyLargeDistance.txt")


def test_round_func_trunc1_and_dimacs_are_same():
    """
    Tests that the DIMACS convention is equivalent to truncating to the first
    decimal.
    """
    trunc1 = read("data/RC208.txt", "solomon", "trunc1")
    dimacs = read("data/RC208.txt", "solomon", "dimacs")

    trunc1_dist = trunc1.distance_matrix()
    dimacs_dist = dimacs.distance_matrix()
    assert_equal(trunc1_dist, dimacs_dist)

    trunc1_dur = trunc1.duration_matrix()
    dimacs_dur = trunc1.duration_matrix()
    assert_equal(trunc1_dur, dimacs_dur)


def test_round_func_round_nearest():
    """
    Tests rounding to the nearest integer works well for the RC208 instance,
    which has Euclidean distances computed from integer coordinates. Since the
    instance is large, we'll test one particular distance.
    """
    data = read("data/RC208.txt", "solomon", "round")

    # We're going to test dist(0, 1) and dist(1, 0), which should be the same
    # since the distances are symmetric/Euclidean.
    assert_equal(data.client(0).x, 40)
    assert_equal(data.client(0).y, 50)

    assert_equal(data.client(1).x, 25)
    assert_equal(data.client(1).y, 85)

    # Compute the distance, and assert that it is indeed correctly rounded.
    dist = sqrt((40 - 25) ** 2 + (85 - 50) ** 2)
    assert_equal(data.dist(0, 1), round(dist))
    assert_equal(data.dist(1, 0), round(dist))
