from numpy.testing import assert_

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import (
    Exchange11,
    LocalSearch,
    NeighbourhoodParams,
    SwapStar,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_swap_star_identifies_more_moves_than_regular_swap():
    """
    SWAP* can move two clients to any position in the routes, whereas regular
    swap ((1, 1)-exchange) must reinsert each client in the other's position.
    Thus, SWAP* should be able to identify additional improving moves beyond
    those of the regular (1, 1)-exchange operator.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    # For a fair comparison we should not hamper the node operator with
    # granularity restrictions.
    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    ls.add_node_operator(Exchange11(data, pm))
    ls.add_route_operator(SwapStar(data, pm))

    for _ in range(10):  # repeat a few times to really make sure
        individual = Individual(data, pm, rng)

        swap_individual = ls.search(individual)
        swap_star_individual = ls.intensify(
            swap_individual, overlap_tolerance_degrees=360
        )

        # The regular swap operator should have been able to improve the random
        # individual. After swap gets stuck, SWAP* should still be able to
        # further improve the individual.
        assert_(swap_individual.cost() < individual.cost())
        assert_(swap_star_individual.cost() < swap_individual.cost())
