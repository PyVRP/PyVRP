from dataclasses import dataclass, field
from time import perf_counter
from typing import List, cast

import numpy as np

from .Individual import Individual
from .Population import Population, SubPopulation


@dataclass
class _Datum:
    """
    Single subpopulation data point.
    """

    size: int
    avg_diversity: float
    best_cost: float
    avg_cost: float
    avg_num_routes: float


@dataclass
class Statistics:
    """
    The Statistics object tracks various (population-level) statistics of
    genetic algorithm runs. This can be helpful in analysing the algorithm's
    performance.
    """

    runtimes: List[float] = field(default_factory=list)
    num_iterations: int = 0
    feas_stats: List[_Datum] = field(default_factory=list)
    infeas_stats: List[_Datum] = field(default_factory=list)

    def __post_init__(self):
        self._clock = perf_counter()

    def collect_from(self, population: Population):
        """
        Collects statistics from the given population object.

        Parameters
        ----------
        population
            Population instance to collect statistics from.
        """
        start = self._clock
        self._clock = perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        feas_subpop = population.feasible_subpopulation
        feas_datum = self._collect_from_subpop(feas_subpop)
        self.feas_stats.append(feas_datum)

        infeas_subpop = population.infeasible_subpopulation
        infeas_datum = self._collect_from_subpop(infeas_subpop)
        self.infeas_stats.append(infeas_datum)

    def _collect_from_subpop(self, subpop: SubPopulation) -> _Datum:
        if not subpop:  # empty, so many statistics cannot be collected
            return _Datum(
                size=0,
                avg_diversity=np.nan,
                best_cost=np.nan,
                avg_cost=np.nan,
                avg_num_routes=np.nan,
            )

        size = len(subpop)
        individuals = [cast(Individual, indiv) for indiv, *_ in subpop]
        costs = [individual.cost() for individual in individuals]
        num_routes = [individual.num_routes() for individual in individuals]
        diversities = [subpop.avg_distance_closest(idx) for idx in range(size)]

        return _Datum(
            size=size,
            avg_diversity=np.mean(diversities),
            best_cost=np.min(costs),
            avg_cost=np.mean(costs),
            avg_num_routes=np.mean(num_routes),
        )
