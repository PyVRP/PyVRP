from __future__ import annotations

import time
from dataclasses import dataclass
from importlib.metadata import version
from typing import TYPE_CHECKING, Callable, Collection

from pyvrp.Result import Result
from pyvrp.Statistics import Statistics

if TYPE_CHECKING:
    from pyvrp.PenaltyManager import PenaltyManager
    from pyvrp.Population import Population
    from pyvrp._pyvrp import (
        CostEvaluator,
        ProblemData,
        RandomNumberGenerator,
        Solution,
    )
    from pyvrp.search.SearchMethod import SearchMethod
    from pyvrp.stop.StoppingCriterion import StoppingCriterion


@dataclass
class GeneticAlgorithmParams:
    """
    Parameters for the genetic algorithm.

    Parameters
    ----------
    repair_probability
        Probability (in :math:`[0, 1]`) of repairing an infeasible solution.
        If the reparation makes the solution feasible, it is also added to
        the population in the same iteration.
    nb_iter_no_improvement
        Number of iterations without any improvement needed before a restart
        occurs.
    log
        Whether to print logs.

    Attributes
    ----------
    repair_probability
        Probability of repairing an infeasible solution.
    nb_iter_no_improvement
        Number of iterations without improvement before a restart occurs.
    log
        Whether to print logs.

    Raises
    ------
    ValueError
        When ``repair_probability`` is not in :math:`[0, 1]`, or
        ``nb_iter_no_improvement`` is negative.
    """

    repair_probability: float = 0.80
    nb_iter_no_improvement: int = 20_000
    log: bool = False

    def __post_init__(self):
        if not 0 <= self.repair_probability <= 1:
            raise ValueError("repair_probability must be in [0, 1].")

        if self.nb_iter_no_improvement < 0:
            raise ValueError("nb_iter_no_improvement < 0 not understood.")


class GeneticAlgorithm:
    """
    Creates a GeneticAlgorithm instance.

    Parameters
    ----------
    data
        Data object describing the problem to be solved.
    penalty_manager
        Penalty manager to use.
    rng
        Random number generator.
    population
        Population to use.
    search_method
        Search method to use.
    crossover_op
        Crossover operator to use for generating offspring.
    initial_solutions
        Initial solutions to use to initialise the population.
    params
        Genetic algorithm parameters. If not provided, a default will be used.

    Raises
    ------
    ValueError
        When the population is empty.
    """

    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: RandomNumberGenerator,
        population: Population,
        search_method: SearchMethod,
        crossover_op: Callable[
            [
                tuple[Solution, Solution],
                ProblemData,
                CostEvaluator,
                RandomNumberGenerator,
            ],
            Solution,
        ],
        initial_solutions: Collection[Solution],
        params: GeneticAlgorithmParams = GeneticAlgorithmParams(),
    ):
        if len(initial_solutions) == 0:
            raise ValueError("Expected at least one initial solution.")

        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._pop = population
        self._search = search_method
        self._crossover = crossover_op
        self._initial_solutions = initial_solutions
        self._params = params

        # Find best feasible initial solution if any exist, else set a random
        # infeasible solution (with infinite cost) as the initial best.
        self._best = min(initial_solutions, key=self._cost_evaluator.cost)

    @property
    def _cost_evaluator(self) -> CostEvaluator:
        return self._pm.get_cost_evaluator()

    def run(self, stop: StoppingCriterion):
        """
        Runs the genetic algorithm with the provided stopping criterion.

        Parameters
        ----------
        stop
            Stopping criterion to use. The algorithm runs until the first time
            the stopping criterion returns ``True``.

        Returns
        -------
        Result
            A Result object, containing statistics and the best found solution.
        """

        if self._params.log:
            print(_log_start(self._data))

        start = time.perf_counter()
        stats = Statistics()
        iters = 0
        iters_no_improvement = 1

        for sol in self._initial_solutions:
            self._pop.add(sol, self._cost_evaluator)

        while not stop(self._cost_evaluator.cost(self._best)):
            iters += 1

            if iters_no_improvement == self._params.nb_iter_no_improvement:
                iters_no_improvement = 1
                self._pop.clear()

                for sol in self._initial_solutions:
                    self._pop.add(sol, self._cost_evaluator)

            curr_best = self._cost_evaluator.cost(self._best)

            parents = self._pop.select(self._rng, self._cost_evaluator)
            offspring = self._crossover(
                parents, self._data, self._cost_evaluator, self._rng
            )
            self._improve_offspring(offspring)

            new_best = self._cost_evaluator.cost(self._best)

            if new_best < curr_best:
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            stats.collect_from(self._pop, self._cost_evaluator)

            if self._params.log and iters % 500 == 0:
                print(_log_solver_entry(curr_best, stats))

        end = time.perf_counter() - start

        if self._params.log:
            print(_log_summary(iters, end, curr_best))

        return Result(self._best, stats, iters, end)

    def _improve_offspring(self, sol: Solution):
        def is_new_best(sol):
            cost = self._cost_evaluator.cost(sol)
            best_cost = self._cost_evaluator.cost(self._best)
            return cost < best_cost

        def add_and_register(sol):
            self._pop.add(sol, self._cost_evaluator)
            self._pm.register_load_feasible(not sol.has_excess_load())
            self._pm.register_time_feasible(not sol.has_time_warp())

        sol = self._search(sol, self._cost_evaluator)
        add_and_register(sol)

        if is_new_best(sol):
            self._best = sol

        # Possibly repair if current solution is infeasible. In that case, we
        # penalise infeasibility more using a penalty booster.
        if (
            not sol.is_feasible()
            and self._rng.rand() < self._params.repair_probability
        ):
            sol = self._search(sol, self._pm.get_booster_cost_evaluator())

            if sol.is_feasible():
                add_and_register(sol)

            if is_new_best(sol):
                self._best = sol


def _centerize(msg: str, length=82) -> str:
    padding = "-" * ((length - len(msg)) // 2)
    ending = "-" if (length - len(msg)) % 2 else ""
    return f"{padding} {msg} {padding}{ending}"


def _pluralize(num: int, word: str) -> str:
    return f"{num} {word}{'s' if num != 1 else ''}"


def _log_start(data: ProblemData) -> str:
    msg = _centerize(f"PyVRP v{version('pyvrp')}") + "\n\n"

    msg += "Solving an instance with "
    msg += f"{_pluralize(data.num_depots, 'depot')}, "
    msg += f"{_pluralize(data.num_clients, 'client')}, "
    msg += f"and {_pluralize(data.num_vehicles, 'vehicle')}.\n\n"

    msg += _centerize("Solver progress") + "\n\n"
    msg += f"{'Statistics':^26} | {'Feasible':^26} | {'Infeasible':^26}\n"
    msg += f"{'Iters':>7} {'T (s)':>7} {'Best':>10} | "
    msg += f"{'Size':>4} {'Avg':>10} {'Best':>10} | "
    msg += f"{'Size':>4} {'Avg':>10} {'Best':>10}\n"

    return msg


def _log_solver_entry(global_best: float, stats: Statistics) -> str:
    iters = stats.num_iterations
    elapsed = round(sum(stats.runtimes), 2)

    msg = f"{iters:>7} {elapsed:>7} {global_best:>10} | "

    def _format_pop_stats(pop):
        size = pop.size
        avg_cost = round(pop.avg_cost, 2)
        best_cost = round(pop.best_cost, 2)
        return f"{size:>4} {avg_cost:>10} {best_cost:>10}"

    msg += _format_pop_stats(stats.feas_stats[-1]) + " | "
    msg += _format_pop_stats(stats.infeas_stats[-1])

    return msg


def _log_summary(iters: int, end: float, best_cost: float) -> str:
    msg = "\n" + _centerize("Summary") + "\n\n"
    msg += (
        f"Search terminated after {iters} iterations and {end:.2f} seconds.\n"
    )
    msg += f"Best-found solution has cost {best_cost}.\n\n"
    msg += _centerize("End")

    return msg
