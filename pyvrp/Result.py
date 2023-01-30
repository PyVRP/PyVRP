from dataclasses import dataclass

from pyvrp._lib.hgspy import Individual, Statistics


@dataclass
class Result:
    best: Individual
    stats: Statistics
    num_iterations: int
    runtime: float

    def __post_init__(self):
        if not self.best.is_feasible():
            raise ValueError("Cannot construct an infeasible Result.")

        if self.num_iterations < 0:
            raise ValueError("Negative number of iterations not understood.")

        if self.runtime < 0:
            raise ValueError("Negative runtime not understood.")
