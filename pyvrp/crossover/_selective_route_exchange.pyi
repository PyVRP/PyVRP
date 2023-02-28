from typing import Tuple

from pyvrp._Individual import Individual
from pyvrp._PenaltyManager import PenaltyManager
from pyvrp._ProblemData import ProblemData
from pyvrp._XorShift128 import XorShift128

def selective_route_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    penalty_manager: PenaltyManager,
    rng: XorShift128,
) -> Individual:
    """
    This crossover operator due to Nagata and Kobayashi (2010) combines routes
    from both parents to generate a new offspring solution. It does this by
    carefully selecting routes from the second parent that could be exchanged
    with routes from the first parent. After exchanging these routes, the
    resulting offspring solution is repaired using a greedy repair strategy.

    Parameters
    ----------
    parents
        The two parent solutions to create an offspring from.
    data
        The problem instance.
    penalty_manager
        The penalty manager instance to be used during the greedy repair step.
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
