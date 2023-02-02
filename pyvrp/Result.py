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

    def plot(self):
        pass

    def plot_population(self):
        pass

    def plot_objectives(self):
        pass

    def plot_instance(self):
        pass

    def __str__(self) -> str:
        summary = [
            "Solution results",
            "================",
            f"   objective: {self.cost():.2f}",
            f"# iterations: {self.num_iterations}",
            f"    run-time: {self.runtime:.2f} seconds\n",
        ]

        routes = [
            "Routes",
            "======",
        ]

        for idx, route in enumerate(self.best.get_routes()):
            if route:
                routes.append(f"Route {idx:>2}: {route}")

        return "\n".join(summary + routes)
