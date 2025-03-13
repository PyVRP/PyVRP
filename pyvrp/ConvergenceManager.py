from __future__ import annotations

from dataclasses import dataclass
from statistics import fmean

import numpy as np


@dataclass
class ConvergenceParams:
    solutions_between_updates: int = 100
    penalty_increase: int = 2
    penalty_decrease: int = 1
    target_pairs: float = 15


class ConvergenceManager:
    MIN_PENALTY: int = 1
    MAX_PENALTY: int = 1_000

    def __init__(
        self,
        initial_num_destroy: int,
        params: ConvergenceParams = ConvergenceParams(),
    ):
        self._num_destroy = initial_num_destroy
        self._params = params
        self._history = []

    def _compute(self, mean) -> int:
        diff = self._params.target_pairs - mean

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
