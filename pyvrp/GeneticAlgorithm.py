import time
from dataclasses import dataclass
from typing import Callable, Tuple

from pyvrp.educate import LocalSearch
from pyvrp.stop import StoppingCriterion

from .Individual import Individual
from .PenaltyManager import PenaltyManager
from .Population import Population
from .ProblemData import ProblemData
from .Result import Result
from .Statistics import Statistics
from .XorShift128 import XorShift128

_Parents = Tuple[Individual, Individual]
CrossoverOperator = Callable[
    [_Parents, ProblemData, PenaltyManager, XorShift128], Individual
]


@dataclass
class GeneticAlgorithmParams:
    collect_statistics: bool = False
    repair_probability: float = 0.80
    intensification_probability: float = 0.15
    intensify_on_best: bool = True
    nb_iter_no_improvement: int = 20_000

    def __post_init__(self):
        if not 0 <= self.repair_probability <= 1:
            raise ValueError("repair_probability must be in [0, 1].")

        if not 0 <= self.intensification_probability <= 1:
            raise ValueError("intensification_probability must be in [0, 1].")

        if self.nb_iter_no_improvement < 0:
            raise ValueError("nb_iter_no_improvement < 0 not understood.")


class GeneticAlgorithm:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        population: Population,
        local_search: LocalSearch,
        crossover_op: CrossoverOperator,
        params: GeneticAlgorithmParams = GeneticAlgorithmParams(),
    ):
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
        local_search
            Local search instance to use.
        crossover_op
            Crossover operator to use for generating offspring.
        params, optional
            Genetic algorithm parameters. If not provided, a default will be
            used.
        """
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._pop = population
        self._ls = local_search
        self._op = crossover_op
        self._params = params

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

        while not stop(self._pop.get_best_found()):
            iters += 1

            if iters_no_improvement == self._params.nb_iter_no_improvement:
                self._pop.restart()
                iters_no_improvement = 1

            curr_best = self._pop.get_best_found().cost()

            parents = self._pop.select()
            offspring = self._op(parents, self._data, self._pm, self._rng)
            self._educate(offspring)

            new_best = self._pop.get_best_found().cost()

            if new_best < curr_best:
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            if self._params.collect_statistics:
                stats.collect_from(self._pop)

        end = time.perf_counter() - start
        return Result(self._pop.get_best_found(), stats, iters, end)

    def _educate(self, individual: Individual):
        intensify = self._rng.rand() < self._params.intensification_probability

        # HACK We keep searching and intensifying to mimic the local search
        # implementation of HGS-CVRP and HGS-VRPTW
        while True:
            self._ls.search(individual)

            if intensify:
                cost = individual.cost()
                self._ls.intensify(individual)

                # Intensification improved the solution, so we search again.
                if individual.cost() < cost:
                    continue

            break

        # Only intensify feasible, new best solutions. See also the repair
        # step below.
        if (
            self._params.intensify_on_best
            and individual.is_feasible()
            and individual.cost() < self._pop.get_best_found().cost()
        ):
            self._ls.intensify(individual)

        self._pop.add(individual)
        self._pm.register_load_feasible(not individual.has_excess_capacity())
        self._pm.register_time_feasible(not individual.has_time_warp())

        # Possibly repair if current solution is infeasible. In that case, we
        # penalise infeasibility more using a penalty booster.
        if (
            not individual.is_feasible()
            and self._rng.rand() < self._params.repair_probability
        ):
            best_found = self._pop.get_best_found()

            with self._pm.get_penalty_booster() as booster:  # noqa
                # HACK We keep searching and intensifying to mimic the local
                # search implementation of HGS-CVRP and HGS-VRPTW
                while True:
                    self._ls.search(individual)

                    if intensify:
                        cost = individual.cost()
                        self._ls.intensify(individual)

                        if individual.cost() < cost:
                            continue

                    break

                if individual.is_feasible():
                    if (
                        self._params.intensify_on_best
                        and individual.cost() < best_found.cost()
                    ):
                        self._ls.intensify(individual)

                    self._pop.add(individual)

                    # We already know the individual is feasible, so load and
                    # time constraints are both satisfied.
                    self._pm.register_load_feasible(True)
                    self._pm.register_time_feasible(True)
