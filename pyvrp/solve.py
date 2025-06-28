from __future__ import annotations

from typing import TYPE_CHECKING

import tomli

import pyvrp.search
from pyvrp.GeneticAlgorithm import GeneticAlgorithm, GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyManager, PenaltyParams
from pyvrp.Population import Population, PopulationParams
from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    NeighbourhoodParams,
    NodeOperator,
    RouteOperator,
    compute_neighbours,
)

if TYPE_CHECKING:
    import pathlib

    from pyvrp.Result import Result
    from pyvrp.stop import StoppingCriterion


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
    display_interval
        Time (in seconds) between iteration logs. Default 5s.
    """

    def __init__(
        self,
        genetic: GeneticAlgorithmParams = GeneticAlgorithmParams(),
        penalty: PenaltyParams = PenaltyParams(),
        population: PopulationParams = PopulationParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        node_ops: list[type[NodeOperator]] = NODE_OPERATORS,
        route_ops: list[type[RouteOperator]] = ROUTE_OPERATORS,
        display_interval: float = 5.0,
    ):
        self._genetic = genetic
        self._penalty = penalty
        self._population = population
        self._neighbourhood = neighbourhood
        self._node_ops = node_ops
        self._route_ops = route_ops
        self._display_interval = display_interval

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, SolveParams)
            and self.genetic == other.genetic
            and self.penalty == other.penalty
            and self.population == other.population
            and self.neighbourhood == other.neighbourhood
            and self.node_ops == other.node_ops
            and self.route_ops == other.route_ops
            and self.display_interval == other.display_interval
        )

    @property
    def genetic(self):
        return self._genetic

    @property
    def penalty(self):
        return self._penalty

    @property
    def population(self):
        return self._population

    @property
    def neighbourhood(self):
        return self._neighbourhood

    @property
    def node_ops(self):
        return self._node_ops

    @property
    def route_ops(self):
        return self._route_ops

    @property
    def display_interval(self) -> float:
        return self._display_interval

    @classmethod
    def from_file(cls, loc: str | pathlib.Path):
        """
        Loads the solver parameters from a TOML file.
        """
        with open(loc, "rb") as fh:
            data = tomli.load(fh)

        node_ops = NODE_OPERATORS
        if "node_ops" in data:
            node_ops = [getattr(pyvrp.search, op) for op in data["node_ops"]]

        route_ops = ROUTE_OPERATORS
        if "route_ops" in data:
            route_ops = [getattr(pyvrp.search, op) for op in data["route_ops"]]

        return cls(
            GeneticAlgorithmParams(**data.get("genetic", {})),
            PenaltyParams(**data.get("penalty", {})),
            PopulationParams(**data.get("population", {})),
            NeighbourhoodParams(**data.get("neighbourhood", {})),
            node_ops,
            route_ops,
            data.get("display_interval", 5.0),
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
        if node_op.supports(data):
            ls.add_node_operator(node_op(data))

    for route_op in params.route_ops:
        if route_op.supports(data):
            ls.add_route_operator(route_op(data))

    pm = PenaltyManager.init_from(data, params.penalty)
    pop = Population(bpd, params.population)
    init = [
        Solution.make_random(data, rng)
        for _ in range(params.population.min_pop_size)
    ]

    # We use SREX when the instance is a proper VRP; else OX for TSP.
    crossover = srex if data.num_vehicles > 1 else ox

    gen_args = (data, pm, rng, pop, ls, crossover, init, params.genetic)
    algo = GeneticAlgorithm(*gen_args)  # type: ignore
    return algo.run(stop, collect_stats, display, params.display_interval)
