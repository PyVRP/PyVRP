import time
from dataclasses import dataclass
from typing import Callable, Collection, Tuple

from pyvrp.educate.LocalSearch import LocalSearch
from pyvrp.stop import StoppingCriterion

from .Population import Population
from .Result import Result
from .Statistics import Statistics
from ._Individual import Individual
from ._PenaltyManager import PenaltyManager
from ._ProblemData import ProblemData
from ._XorShift128 import XorShift128

_Parents = Tuple[Individual, Individual]
CrossoverOperator = Callable[
    [_Parents, ProblemData, PenaltyManager, XorShift128], Individual
]


@dataclass
class GeneticAlgorithmParams:
    repair_probability: float = 0.80
    collect_statistics: bool = False
    intensify_probability: float = 0.15
    intensify_on_best: bool = True
    nb_iter_no_improvement: int = 20_000

    def __post_init__(self):
        if not 0 <= self.repair_probability <= 1:
            raise ValueError("repair_probability must be in [0, 1].")

        if not 0 <= self.intensify_probability <= 1:
            raise ValueError("intensify_probability must be in [0, 1].")

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
    local_search
        Local search instance to use.
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
        rng: XorShift128,
        population: Population,
        local_search: LocalSearch,
        crossover_op: CrossoverOperator,
        initial_solutions: Collection[Individual],
        params: GeneticAlgorithmParams = GeneticAlgorithmParams(),
    ):
        if len(initial_solutions) == 0:
            raise ValueError("Expected at least one initial solution.")

        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._pop = population
        self._ls = local_search
        self._op = crossover_op
        self._initial_solutions = initial_solutions
        self._params = params

        # Find best feasible initial solution if any exist, else set the best
        # infeasible solution as the initial best.
        feas = [indiv for indiv in initial_solutions if indiv.is_feasible()]
        sols = feas if feas else initial_solutions
        self._best = min(sols, key=lambda indiv: indiv.cost())

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
        start = time.perf_counter()
        stats = Statistics()
        iters = 0
        iters_no_improvement = 1

        for individual in self._initial_solutions:
            self._pop.add(individual)

        while not stop(self._best):
            iters += 1

            if iters_no_improvement == self._params.nb_iter_no_improvement:
                iters_no_improvement = 1
                self._pop.clear()

                for individual in self._initial_solutions:
                    self._pop.add(individual)

            curr_best = self._best.cost()

            parents = self._pop.select(self._rng)
            offspring = self._op(parents, self._data, self._pm, self._rng)
            self._educate(offspring)

            new_best = self._best.cost()

            if new_best < curr_best:
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            if self._params.collect_statistics:
                stats.collect_from(self._pop)

        end = time.perf_counter() - start
        return Result(self._best, stats, iters, end)

    def _educate(self, individual: Individual):
        def is_new_best(indiv):
            return indiv.is_feasible() and indiv.cost() < self._best.cost()

        def add_and_register(indiv):
            self._pop.add(indiv)
            self._pm.register_load_feasible(not indiv.has_excess_capacity())
            self._pm.register_time_feasible(not indiv.has_time_warp())

        intensify_prob = self._params.intensify_probability
        should_intensify = self._rng.rand() < intensify_prob

        individual = self._ls.run(individual, self._pm, should_intensify)

        if is_new_best(individual):
            self._best = individual

            # Only intensify feasible, new best solutions. See also the repair
            # step below. TODO Refactor to on_best callback (see issue #111)
            if self._params.intensify_on_best:
                individual = self._ls.intensify(
                    individual, self._pm, overlap_tolerance_degrees=360
                )

                if is_new_best(individual):
                    self._best = individual

        add_and_register(individual)

        # Possibly repair if current solution is infeasible. In that case, we
        # penalise infeasibility more using a penalty booster.
        if (
            not individual.is_feasible()
            and self._rng.rand() < self._params.repair_probability
        ):
            with self._pm.get_penalty_booster():
                should_intensify = self._rng.rand() < intensify_prob
                individual = self._ls.run(
                    individual, self._pm, should_intensify
                )

                if is_new_best(individual):
                    self._best = individual

                    # TODO Refactor to on_best callback (see issue #111)
                    if self._params.intensify_on_best:
                        individual = self._ls.intensify(
                            individual, self._pm, overlap_tolerance_degrees=360
                        )

                        if is_new_best(individual):
                            self._best = individual

            if individual.is_feasible():
                add_and_register(individual)
