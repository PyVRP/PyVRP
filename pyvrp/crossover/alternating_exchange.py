from typing import Tuple

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._Individual import Individual
from pyvrp._ProblemData import ProblemData
from pyvrp._XorShift128 import XorShift128

from .make_giant_tour import make_giant_tour
from .split_giant_tour import split_giant_tour


def alternating_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    rng: XorShift128,
) -> Individual:
    """
    Creates an offspring by selecting alternately the next client of the first
    parent and the next client of the second parent, omitting clients already
    present in the offspring.

    Parameters
    ----------
    parents
        The two parent solutions to create an offspring from.
    data
        The problem instance.
    cost_evaluator
        The cost evaluator to be used during the greedy repair step.
    rng
        The random number generator to use.

    Returns
    -------
    Individual
        A new offspring.

    References
    ----------
    .. [1] Nagata, Y., & Kobayashi, S. (2010). A Memetic Algorithm for the
           Pickup and Delivery Problem with Time Windows Using Selective Route
           Exchange Crossover. *Parallel Problem Solving from Nature*, PPSN XI,
           536 - 545.
    """
    tour1 = make_giant_tour(parents[0].get_routes())
    tour2 = make_giant_tour(parents[1].get_routes())

    visited = set()
    tour = []

    for idx in range(data.num_clients):
        if tour1[idx] not in visited:
            tour.append(tour1[idx])
            visited.add(tour1[idx])

        if tour2[idx] not in visited:
            tour.append(tour2[idx])
            visited.add(tour2[idx])

    return Individual(data, split_giant_tour(tour, data))
