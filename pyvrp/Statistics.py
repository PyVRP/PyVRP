import csv
from dataclasses import dataclass, field, fields
from math import nan
from statistics import fmean
from time import perf_counter
from typing import List, Union

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
                avg_diversity=nan,
                best_cost=nan,
                avg_cost=nan,
                avg_num_routes=nan,
            )

        size = len(subpop)
        costs = [individual.cost() for individual, *_ in subpop]
        num_routes = [individual.num_routes() for individual, *_ in subpop]
        diversities = [subpop.avg_distance_closest(idx) for idx in range(size)]

        return _Datum(
            size=size,
            avg_diversity=fmean(diversities),
            best_cost=min(costs),
            avg_cost=fmean(costs),
            avg_num_routes=fmean(num_routes),
        )

    def to_csv(self, where: str, dialect: Union[csv.Dialect, str] = "unix"):
        """
        Writes this statistics object to the given location, as a CSV file.

        Parameters
        ----------
        where
            Filesystem location to write to.
        dailect
            CSV dialect to use. Defaults to 'unix'.
        """
        field_names = [f.name for f in fields(_Datum)]
        feas_fields = ["feas_" + field for field in field_names]
        infeas_fields = ["infeas_" + field for field in field_names]

        feas_data = [
            {f: v for f, v in zip(feas_fields, vars(datum).values())}
            for datum in self.feas_stats
        ]

        infeas_data = [
            {f: v for f, v in zip(infeas_fields, vars(datum).values())}
            for datum in self.infeas_stats
        ]

        with open(where, "w") as fh:
            header = ["runtime"] + feas_fields + infeas_fields
            writer = csv.DictWriter(fh, header, dialect=dialect)
            writer.writeheader()

            for idx in range(self.num_iterations):
                row = dict(runtime=self.runtimes[idx])
                row.update(feas_data[idx])
                row.update(infeas_data[idx])

                writer.writerow(row)
