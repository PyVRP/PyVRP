from dataclasses import dataclass

from pyvrp._lib.hgspy import Individual

from .Statistics import Statistics

@dataclass
class Result:
    """
    Stores the outcomes of a single run. An instance of this class is returned
    once the GeneticAlgorithm completes.

    Parameters
    ----------
    best
        The best observed solution.
    stats
        A Statistics object containing runtime statistics. These are only
        collected and available if statistics were collected for the given run.
    num_iterations
        Number of iterations performed by the genetic algorithm.
    runtime
        Total runtime of the main genetic algorithm loop.

    Raises
    ------
    ValueError
        When the number of iterations or runtime are negative.
    """

    best: Individual
    stats: Statistics
    num_iterations: int
    runtime: float

    def __post_init__(self):
        if self.num_iterations < 0:
            raise ValueError("Negative number of iterations not understood.")

        if self.runtime < 0:
            raise ValueError("Negative runtime not understood.")

    def cost(self) -> float:
        """
        Returns the cost (objective) value of the best solution.

        Returns
        -------
        float
            Objective value.
        """
        return self.best.cost()

    def is_feasible(self) -> bool:
        """
        Returns whether the best solution is feasible.

        Returns
        -------
        bool
            True when the solution is feasible, False otherwise.
        """
        return self.best.is_feasible()

    def has_statistics(self) -> bool:
        """
        Returns whether detailed statistics were collected. If statistics are
        not availabe, the plotting methods cannot be used.

        Returns
        -------
        bool
            True when detailed statistics are available, False otherwise.
        """
        return (
            self.num_iterations > 0
            and self.num_iterations == self.stats.num_iterations
        )

    def __str__(self) -> str:
        summary = [
            "Solution results",
            "================",
            f"    # routes: {self.best.num_routes()}",
            f"   objective: {self.cost():.2f}",
            f"    feasible? {self.is_feasible()}",
            f"# iterations: {self.num_iterations}",
            f"    run-time: {self.runtime:.2f} seconds",
            "",
            "Routes",
            "------",
        ]

        for idx, route in enumerate(self.best.get_routes(), 1):
            if route:
                summary.append(f"Route {idx:>2}: {route}")

        return "\n".join(summary)
