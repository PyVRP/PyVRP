from __future__ import annotations

from dataclasses import dataclass, field
from typing import TYPE_CHECKING

from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population
from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.search import LocalSearch, compute_neighbours

if TYPE_CHECKING:
    import pathlib

    from pyvrp.Result import Result
    from pyvrp.stop import StoppingCriterion

from typing import Iterable, Type, Union

import tomli

import pyvrp.search
from pyvrp.GeneticAlgorithm import GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.Population import PopulationParams
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    NeighbourhoodParams,
    NodeOperator,
    RouteOperator,
)


@dataclass
class SolveParams:
    """
    Solver parameters for PyVRP's hybrid genetic search algorithm.

    Parameters
    ----------
    genetic
        Genetic algorithm parameters.
    penalty
        Penalty parameters.
    population
        Population parameters.
    neighbourhood
        Neighbourhood parameters.
    node_ops
        Node operators to use in the search.
    route_ops
        Route operators to use in the search.
    """

    genetic: GeneticAlgorithmParams = field(
        default_factory=GeneticAlgorithmParams
    )
    penalty: PenaltyParams = field(default_factory=PenaltyParams)
    population: PopulationParams = field(default_factory=PopulationParams)
    neighbourhood: NeighbourhoodParams = field(
        default_factory=NeighbourhoodParams
    )
    node_ops: Iterable[Type[NodeOperator]] = field(
        default_factory=lambda: NODE_OPERATORS
    )
    route_ops: Iterable[Type[RouteOperator]] = field(
        default_factory=lambda: ROUTE_OPERATORS
    )

    @classmethod
    def from_file(cls, loc: Union[str, pathlib.Path]):
        """
        Loads the solver parameters from a TOML file.
        """
        with open(loc, "rb") as fh:
            data = tomli.load(fh)

        gen_params = GeneticAlgorithmParams(**data.get("genetic", {}))
        pen_params = PenaltyParams(**data.get("penalty", {}))
        pop_params = PopulationParams(**data.get("population", {}))
        nb_params = NeighbourhoodParams(**data.get("neighbourhood", {}))
        node_ops = [getattr(pyvrp.search, op) for op in data["node_ops"]]
        route_ops = [getattr(pyvrp.search, op) for op in data["route_ops"]]

        return cls(
            gen_params, pen_params, pop_params, nb_params, node_ops, route_ops
        )


def solve(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    collect_stats: bool = True,
    display: bool = False,
    params: SolveParams = SolveParams(),
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
    collect_stats
        Whether to collect statistics about the solver's progress. Default
        ``True``.
    display
        Whether to display information about the solver progress. Default
        ``False``. Progress information is only available when
        ``collect_stats`` is also set, which it is by default.
    params
        Solver parameters to use. If not provided, a default will be used.

    Returns
    -------
    Result
        A Result object, containing statistics (if collected) and the best
        found solution.
    """
    rng = RandomNumberGenerator(seed=seed)
    neighbours = compute_neighbours(data, params.neighbourhood)
    ls = LocalSearch(data, rng, neighbours)

    for node_op in params.node_ops:
        ls.add_node_operator(node_op(data))

    for route_op in params.route_ops:
        ls.add_route_operator(route_op(data))

    pm = PenaltyManager(params.penalty)
    pop = Population(bpd, params.population)
    init = [
        Solution.make_random(data, rng)
        for _ in range(params.population.min_pop_size)
    ]

    # We use SREX when the instance is a proper VRP; else OX for TSP.
    crossover = srex if data.num_vehicles > 1 else ox

    gen_args = (data, pm, rng, pop, ls, crossover, init, params.genetic)
    algo = GeneticAlgorithm(*gen_args)  # type: ignore
    return algo.run(stop, collect_stats, display)
