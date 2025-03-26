from __future__ import annotations

from dataclasses import dataclass
from statistics import fmean
from time import perf_counter

import numpy as np


@dataclass
class ConvergenceParams:
    solutions_between_updates: int = 100
    penalty_increase: int = 1
    penalty_decrease: int = 5
    target_pairs_max: float = 20
    target_pairs_min: float = 10


class ConvergenceManager:
    MIN_PENALTY: int = 1
    MAX_PENALTY: int = 1_000

    def __init__(
        self,
        initial_num_destroy: int,
        max_runtime: float,
        params: ConvergenceParams = ConvergenceParams(),
    ):
        self._num_destroy = initial_num_destroy
        self._params = params
        self._max_runtime = max_runtime

        self._history = []
        self._start_time = None

    @property
    def target_pairs(self) -> float:
        if self._start_time is None:
            self._start_time = perf_counter()

        pct_time = (perf_counter() - self._start_time) / self._max_runtime
        delta = self._params.target_pairs_max - self._params.target_pairs_min
        return self._params.target_pairs_min + delta * (1 - pct_time)

    def _compute(self, mean) -> int:
        diff = self.target_pairs - mean

        if abs(diff) <= 1:  # +/-1 tolerance
            return self._num_destroy

        new_penalty = self._num_destroy
        if diff > 0:
            new_penalty += self._params.penalty_increase
        else:
            new_penalty -= self._params.penalty_decrease

        return np.clip(new_penalty, self.MIN_PENALTY, self.MAX_PENALTY)

    @property
    def num_destroy(self) -> int:
        return self._num_destroy

    def register(self, broken_pairs: int):
        self._history.append(broken_pairs)

        if len(self._history) >= self._params.solutions_between_updates:
            mean = fmean(self._history)
            self._num_destroy = self._compute(mean)
            self._history = []
