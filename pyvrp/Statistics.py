import csv
from dataclasses import dataclass, fields
from math import nan
from pathlib import Path
from statistics import fmean
from time import perf_counter
from typing import Union

from pyvrp.Population import Population, SubPopulation
from pyvrp._pyvrp import CostEvaluator

_FEAS_CSV_PREFIX = "feas_"
_INFEAS_CSV_PREFIX = "infeas_"


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

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, _Datum):
            return False

        if self.size == other.size == 0:  # shortcut to avoid comparing NaN
            return True

        return (
            self.size == other.size
            and self.avg_diversity == other.avg_diversity
            and self.best_cost == other.best_cost
            and self.avg_cost == other.avg_cost
            and self.avg_num_routes == other.avg_num_routes
        )


class Statistics:
    """
    The Statistics object tracks various (population-level) statistics of
    genetic algorithm runs. This can be helpful in analysing the algorithm's
    performance.

    Parameters
    ----------
    collect_stats
        Whether to collect statistics at all. This can be turned off to avoid
        excessive memory use on long runs.
    """

    runtimes: list[float]
    num_iterations: int
    feas_stats: list[_Datum]
    infeas_stats: list[_Datum]

    def __init__(self, collect_stats: bool = True):
        self.runtimes = []
        self.num_iterations = 0
        self.feas_stats = []
        self.infeas_stats = []

        self._clock = perf_counter()
        self._collect_stats = collect_stats

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, Statistics)
            and self._collect_stats == other._collect_stats
            and self.runtimes == other.runtimes
            and self.num_iterations == other.num_iterations
            and self.feas_stats == other.feas_stats
            and self.infeas_stats == other.infeas_stats
        )

    def is_collecting(self) -> bool:
        return self._collect_stats

    def collect_from(
        self, population: Population, cost_evaluator: CostEvaluator
    ):
        """
        Collects statistics from the given population object.

        Parameters
        ----------
        population
            Population instance to collect statistics from.
        cost_evaluator
            CostEvaluator used to compute costs for solutions.
        """
        if not self._collect_stats:
            return

        start = self._clock
        self._clock = perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        # The following lines access private members of the population, but in
        # this case that is mostly OK: we really want to have that access to
        # enable detailed statistics logging.
        feas_subpop = population._feas  # noqa: SLF001
        feas_datum = self._collect_from_subpop(feas_subpop, cost_evaluator)
        self.feas_stats.append(feas_datum)

        infeas_subpop = population._infeas  # noqa: SLF001
        infeas_datum = self._collect_from_subpop(infeas_subpop, cost_evaluator)
        self.infeas_stats.append(infeas_datum)

    def _collect_from_subpop(
        self, subpop: SubPopulation, cost_evaluator: CostEvaluator
    ) -> _Datum:
        if not subpop:  # empty, so many statistics cannot be collected
            return _Datum(
                size=0,
                avg_diversity=nan,
                best_cost=nan,
                avg_cost=nan,
                avg_num_routes=nan,
            )

        size = len(subpop)
        costs = [
            cost_evaluator.penalised_cost(item.solution) for item in subpop
        ]
        num_routes = [item.solution.num_routes() for item in subpop]
        diversities = [item.avg_distance_closest() for item in subpop]

        return _Datum(
            size=size,
            avg_diversity=fmean(diversities),
            best_cost=min(costs),
            avg_cost=fmean(costs),
            avg_num_routes=fmean(num_routes),
        )

    @classmethod
    def from_csv(cls, where: Union[Path, str], delimiter: str = ",", **kwargs):
        """
        Reads a Statistics object from the CSV file at the given filesystem
        location.

        Parameters
        ----------
        where
            Filesystem location to read from.
        delimiter
            Value separator. Default comma.
        kwargs
            Additional keyword arguments. These are passed to
            :class:`csv.DictReader`.

        Returns
        -------
        Statistics
            Statistics object populated with the data read from the given
            filesystem location.
        """
        field2type = {field.name: field.type for field in fields(_Datum)}

        def make_datum(row, prefix) -> _Datum:
            datum = {}

            for name, value in row.items():
                if (field_name := name[len(prefix) :]) in field2type:
                    # If the prefixless name is a field name, cast the row's
                    # value to the appropriate type and add the data.
                    datum[field_name] = field2type[field_name](value)

            return _Datum(**datum)

        with open(where) as fh:
            lines = fh.readlines()

        stats = cls()

        for row in csv.DictReader(lines, delimiter=delimiter, **kwargs):
            stats.runtimes.append(float(row["runtime"]))
            stats.num_iterations += 1
            stats.feas_stats.append(make_datum(row, _FEAS_CSV_PREFIX))
            stats.infeas_stats.append(make_datum(row, _INFEAS_CSV_PREFIX))

        return stats

    def to_csv(
        self,
        where: Union[Path, str],
        delimiter: str = ",",
        quoting: int = csv.QUOTE_MINIMAL,
        **kwargs,
    ):
        """
        Writes this Statistics object to the given location, as a CSV file.

        Parameters
        ----------
        where
            Filesystem location to write to.
        delimiter
            Value separator. Default comma.
        quoting
            Quoting strategy. Default only quotes values when necessary.
        kwargs
            Additional keyword arguments. These are passed to
            :class:`csv.DictWriter`.
        """
        field_names = [f.name for f in fields(_Datum)]
        feas_fields = [_FEAS_CSV_PREFIX + field for field in field_names]
        infeas_fields = [_INFEAS_CSV_PREFIX + field for field in field_names]

        feas_data = [
            {f: v for f, v in zip(feas_fields, vars(datum).values())}
            for datum in self.feas_stats
        ]

        infeas_data = [
            {f: v for f, v in zip(infeas_fields, vars(datum).values())}
            for datum in self.infeas_stats
        ]

        with open(where, "w") as fh:
            header = ["runtime", *feas_fields, *infeas_fields]
            writer = csv.DictWriter(
                fh, header, delimiter=delimiter, quoting=quoting, **kwargs
            )

            writer.writeheader()

            for idx in range(self.num_iterations):
                row = dict(runtime=self.runtimes[idx])
                row.update(feas_data[idx])
                row.update(infeas_data[idx])

                writer.writerow(row)
