import csv
from dataclasses import asdict, dataclass, fields
from pathlib import Path
from time import perf_counter
from typing import Iterator, Literal

from pyvrp._pyvrp import CostEvaluator, Solution


@dataclass(slots=True)
class _Datum:
    """
    Single iteration data point.
    """

    current_cost: int
    current_feas: bool
    candidate_cost: int
    candidate_feas: bool
    best_cost: int
    best_feas: bool


class Statistics:
    """
    Statistics about the search progress.

    Parameters
    ----------
    collect_stats
        Whether to collect statistics at all. This can be turned off to avoid
        excessive memory use on long runs.
    """

    runtimes: list[float]
    num_iterations: int
    data: list[_Datum]

    def __init__(self, collect_stats: bool = True):
        self.runtimes = []
        self.num_iterations = 0
        self.data = []

        self._clock = perf_counter()
        self._collect_stats = collect_stats

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, Statistics)
            and self._collect_stats == other._collect_stats
            and self.runtimes == other.runtimes
            and self.num_iterations == other.num_iterations
            and self.data == other.data
        )

    def __iter__(self) -> Iterator[_Datum]:
        """
        Iterates over the collected data points.
        """
        yield from self.data

    def is_collecting(self) -> bool:
        return self._collect_stats

    def collect(
        self,
        current: Solution,
        candidate: Solution,
        best: Solution,
        cost_evaluator: CostEvaluator,
    ):
        """
        Collect iteration statistics.

        Parameters
        ----------
        current
            The current solution.
        candidate
            The candidate solution.
        best
            The best solution.
        cost_evaluator
            CostEvaluator used to compute costs for solutions.
        """
        if not self._collect_stats:
            return

        start = self._clock
        self._clock = perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        datum = _Datum(
            cost_evaluator.penalised_cost(current),
            current.is_feasible(),
            cost_evaluator.penalised_cost(candidate),
            candidate.is_feasible(),
            cost_evaluator.penalised_cost(best),
            best.is_feasible(),
        )
        self.data.append(datum)

    @classmethod
    def from_csv(cls, where: Path | str, delimiter: str = ",", **kwargs):
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

        def make_datum(row) -> _Datum:
            datum = {}

            for name, value in row.items():
                if name in field2type:
                    if field2type[name] is bool:
                        datum[name] = bool(int(value))
                    else:
                        datum[name] = field2type[name](value)  # type: ignore

            return _Datum(**datum)

        with open(where) as fh:
            lines = fh.readlines()

        stats = cls()

        for row in csv.DictReader(lines, delimiter=delimiter, **kwargs):
            stats.runtimes.append(float(row["runtime"]))
            stats.num_iterations += 1
            stats.data.append(make_datum(row))

        return stats

    def to_csv(
        self,
        where: Path | str,
        delimiter: str = ",",
        quoting: Literal[0, 1, 2, 3] = csv.QUOTE_MINIMAL,
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
        with open(where, "w") as fh:
            header = ["runtime", *(f.name for f in fields(_Datum))]
            writer = csv.DictWriter(
                fh, header, delimiter=delimiter, quoting=quoting, **kwargs
            )
            writer.writeheader()

            for datum, runtime in zip(self.data, self.runtimes):
                row = {
                    f: (int(v) if isinstance(v, bool) else v)  # bool as 0/1
                    for f, v in asdict(datum).items()
                }
                row["runtime"] = runtime
                writer.writerow(row)
