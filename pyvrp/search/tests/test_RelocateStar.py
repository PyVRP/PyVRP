from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    Exchange10,
    LocalSearch,
    NeighbourhoodParams,
    RelocateStar,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_exchange10_and_relocate_star_are_same_large_neighbourhoods():
    """
    With sufficiently large granular neighbourhoods, (1, 0)-Exchange and
    RELOCATE* find the exact same solutions. Only when the granular
    neighbourhood is restricted do these solutions start to differ.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))

    ls.add_node_operator(Exchange10(data))
    ls.add_route_operator(RelocateStar(data))

    for _ in range(10):  # repeat a few times to really make sure
        sol = Solution.make_random(data, rng)
        exchange_sol = ls.search(sol, cost_evaluator)
        relocate_sol = ls.intensify(
            exchange_sol, cost_evaluator, overlap_tolerance=1
        )

        # RELOCATE* applies the best (1, 0)-exchange moves between routes. But
        # when the granular neighbourhood covers the entire client space, that
        # best move has already been evaluated and applied by regular (1,0)
        # exchange. Thus, at this point the solution cannot be improved
        # further by RELOCATE*.
        assert_equal(relocate_sol, exchange_sol)


@mark.parametrize("size", [2, 5, 10])
def test_exchange10_and_relocate_star_differ_small_neighbourhoods(size: int):
    """
    This test restricts the sizes of the granular neighbourhoods, so now
    (1, 0)-Exchange and RELOCATE* should start to differ.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=size)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))

    ls.add_node_operator(Exchange10(data))
    ls.add_route_operator(RelocateStar(data))

    sol = Solution.make_random(data, rng)
    exchange_sol = ls.search(sol, cost_evaluator)
    relocate_sol = ls.intensify(
        exchange_sol, cost_evaluator, overlap_tolerance=1
    )

    # The original solution was not that great, so after (1, 0)-Exchange it
    # should have improved. But that operator is restricted by the size of the
    # granular neighbourhood, which limits the number of operators. RELOCATE*
    # overcomes some of that, and as a result, should be able to improve the
    # solution further.
    current_cost = cost_evaluator.penalised_cost(sol)
    exchange_cost = cost_evaluator.penalised_cost(exchange_sol)
    relocate_cost = cost_evaluator.penalised_cost(relocate_sol)
    assert_(current_cost > exchange_cost)
    assert_(exchange_cost > relocate_cost)
