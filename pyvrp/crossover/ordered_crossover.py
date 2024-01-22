from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.crossover._crossover import ordered_crossover as _ox


def ordered_crossover(
    parents: tuple[Solution, Solution],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    rng: RandomNumberGenerator,
) -> Solution:
    """
    Performs an ordered crossover (OX) operation between the two given parents.
    The clients between two randomly selected indices of the first route are
    copied into a new solution, and any missing clients that are present in the
    second solution are then copied in as well. See [1]_ for details.

    Parameters
    ----------
    parents
        The two parent solutions to create an offspring from.
    data
        The problem instance.
    cost_evaluator
        Cost evaluator object. Unused by this operator.
    rng
        The random number generator to use.

    Returns
    -------
    Solution
        A new offspring.

    Raises
    ------
    ValueError
        When the given data instance does not describe a TSP, particularly,
        when there is more than one vehicle in the data.

    References
    ----------
    .. [1] I. M. Oliver, D. J. Smith, and J. R. C. Holland. 1987. A study of
           permutation crossover operators on the traveling salesman problem.
           In *Proceedings of the Second International Conference on Genetic
           Algorithms on Genetic algorithms and their application*. 224 - 230.
    """
    if data.num_vehicles != 1:
        msg = f"Expected a TSP, got {data.num_vehicles} vehicles instead."
        raise ValueError(msg)

    first, second = parents

    if first.num_clients() == 0:
        return second

    if second.num_clients() == 0:
        return first

    start = rng.randint(data.num_clients)
    end = rng.randint(data.num_clients)

    while end == start and data.num_clients > 1:
        end = rng.randint(data.num_clients)

    return _ox(parents, data, (start, end))
