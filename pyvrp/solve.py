from __future__ import annotations

from typing import TYPE_CHECKING

from pyvrp.Config import Config
from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population
from pyvrp._pyvrp import (
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.search import (
    LocalSearch,
    compute_neighbours,
)

if TYPE_CHECKING:
    from pyvrp.Result import Result
    from pyvrp.stop import StoppingCriterion


def solve(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    config: Config = Config(),
    collect_stats: bool = True,
    display: bool = True,
) -> Result:
    """
    Solves the given problem data instance.

    Parameters
    ----------
    data
        Problem data instance to solve.
    stop
        Stopping criterion to use.
    seed
        Seed value to use for the random number stream. Default 0.
    config
        Configuration to use. If not provided, a default will be used.
    collect_stats
        Whether to collect statistics about the solver's progress. Default
        ``True``.
    display
        Whether to display information about the solver progress. Default
        ``True``. Progress information is only available when
        ``collect_stats`` is also set, which it is by default.

    Returns
    -------
    Result
        A Result object, containing statistics (if collected) and the best
        found solution.
    """
    rng = RandomNumberGenerator(seed=seed)
    neighbours = compute_neighbours(data, config.neighbourhood)
    ls = LocalSearch(data, rng, neighbours)

    for node_op in config.node_ops:
        ls.add_node_operator(node_op(data))

    for route_op in config.route_ops:
        ls.add_route_operator(route_op(data))

    pm = PenaltyManager(config.penalty)
    pop = Population(bpd, config.population)
    init = [
        Solution.make_random(data, rng)
        for _ in range(config.population.min_pop_size)
    ]

    # We use SREX when the instance is a proper VRP; else OX for TSP.
    crossover = srex if data.num_vehicles > 1 else ox

    gen_args = (data, pm, rng, pop, ls, crossover, init, config.genetic)
    algo = GeneticAlgorithm(*gen_args)  # type: ignore
    return algo.run(stop, collect_stats, display)
