import csv
import time
from dataclasses import dataclass


@dataclass
class _Datum:
    """
    Single ILS iteration data point.
    """

    current_cost: float
    current_feas: bool
    candidate_cost: float
    candidate_feas: bool
    best_cost: float
    best_feas: bool
    threshold: float


class Statistics:
    """
    Statistics about the Iterated Local Search progress.
    """

    def __init__(self, collect_stats: bool = True):
        self.runtimes: list[float] = []
        self.num_iterations = 0
        self.data: list[_Datum] = []

        self._clock = time.perf_counter()
        self._collect_stats = collect_stats

    def is_collecting(self) -> bool:
        return self._collect_stats

    def collect(
        self,
        current_cost: float,
        current_feas: bool,
        candidate_cost: float,
        candidate_feas: bool,
        best_cost: float,
        best_feas: bool,
        threshold: float,
    ):
        """
        Collects statistics from the ILS iteration.
        """
        if not self._collect_stats:
            return

        start = self._clock
        self._clock = time.perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        datum = _Datum(
            current_cost,
            current_feas,
            candidate_cost,
            candidate_feas,
            best_cost,
            best_feas,
            threshold,
        )
        self.data.append(datum)

    def to_csv(self, file_path: str):
        """
        Exports the collected statistics to a CSV file.

        Parameters:
            file_path (str): The file path where the CSV will be saved.
        """
        if not self.data:
            print("No data to export.")
            return

        headers = [
            "current_cost",
            "current_feas",
            "candidate_cost",
            "candidate_feas",
            "best_cost",
            "best_feas",
            "threshold",
        ]

        with open(file_path, mode="w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(headers)
            for datum in self.data:
                writer.writerow(
                    [
                        datum.current_cost,
                        datum.current_feas,
                        datum.candidate_cost,
                        datum.candidate_feas,
                        datum.best_cost,
                        datum.best_feas,
                        datum.threshold,
                    ]
                )
