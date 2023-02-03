from numpy.testing import assert_almost_equal

from pyvrp._lib.hgspy import Individual, PenaltyManager
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.tests.helpers import read


def test_broken_pairs_distance():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv1 = Individual(data, pm, [[1, 2, 3, 4], [], []])
    indiv2 = Individual(data, pm, [[1, 2], [3], [4]])
    indiv3 = Individual(data, pm, [[3], [4, 1, 2], []])
    indiv4 = Individual(data, pm, [[4, 3, 2, 1], [], []])

    # BPD of an individual and itself should be zero.
    for indiv in [indiv1, indiv2, indiv3, indiv4]:
        assert_almost_equal(bpd(data, indiv, indiv), 0.0)

    # BPD of indiv1 and indiv2. The two broken pairs are (2, 3) and (3, 4).
    assert_almost_equal(bpd(data, indiv1, indiv2), 0.5)
    assert_almost_equal(bpd(data, indiv2, indiv1), 0.5)

    # BPD of indiv1 and indiv3. The three broken pairs are (0, 1), (2, 3),
    # and (3, 4)
    assert_almost_equal(bpd(data, indiv1, indiv3), 0.75)
    assert_almost_equal(bpd(data, indiv3, indiv1), 0.75)

    # BPD of indiv1 and indiv4. Indiv4 has the reverse route, so all pairs
    # should be broken.
    assert_almost_equal(bpd(data, indiv1, indiv4), 1.0)
    assert_almost_equal(bpd(data, indiv4, indiv1), 1.0)

    # BPD of indiv2 and indiv3. The broken pair is (0, 1).
    assert_almost_equal(bpd(data, indiv2, indiv3), 0.25)
    assert_almost_equal(bpd(data, indiv3, indiv2), 0.25)
