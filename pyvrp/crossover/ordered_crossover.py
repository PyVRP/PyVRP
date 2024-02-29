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
    second route are then copied in as well. See [1]_ for details.

    .. warning::

       This operator explicitly assumes the problem instance is a TSP. You
       should use a different crossover operator if that is not the case.

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
        When the given data instance is not a TSP, particularly, when there is
        more than one vehicle in the data.

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

    # Generate [start, end) indices in the route of the first parent solution.
    # If end < start, the index segment wraps around. Clients in this index
    # segment are copied verbatim into the offspring solution.
    first_route = first.routes()[0]
    start = rng.randint(len(first_route))
    end = rng.randint(len(first_route))

    # When start == end we try to find a different end index, such that the
    # offspring actually inherits something from each parent.
    while start == end and len(first_route) > 1:
        end = rng.randint(len(first_route))

    return _ox(parents, data, (start, end))
