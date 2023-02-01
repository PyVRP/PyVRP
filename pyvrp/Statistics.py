from dataclasses import dataclass, field
from time import perf_counter
from typing import List, Optional

import numpy as np

from .Population import Population, SubPop


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
    feas_stats: List[Optional[_Datum]] = field(default_factory=list)
    infeas_stats: List[Optional[_Datum]] = field(default_factory=list)

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

        feas = self._collect_from_subpop(population, population._feas)
        self.feas_stats.append(feas)

        infeas = self._collect_from_subpop(population, population._infeas)
        self.infeas_stats.append(infeas)

    def _collect_from_subpop(
        self, population: Population, subpop: SubPop
    ) -> Optional[_Datum]:
        if not subpop:
            return None

        costs = [item.cost() for item in subpop]
        num_routes = [item.individual.num_routes() for item in subpop]
        diversities = [
            population.avg_distance_closest(item.individual) for item in subpop
        ]

        return _Datum(
            size=len(subpop),
            avg_diversity=np.mean(diversities),
            best_cost=np.min(costs),
            avg_cost=np.mean(costs),
            avg_num_routes=np.mean(num_routes),
        )
