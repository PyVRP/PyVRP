import time
from dataclasses import dataclass
from typing import Callable, List, Tuple

from pyvrp._lib.hgspy import (
    Individual,
    LocalSearch,
    PenaltyManager,
    ProblemData,
    XorShift128,
)
from pyvrp.stop import StoppingCriterion

from .Population import Population
from .Result import Result
from .Statistics import Statistics

_Parents = Tuple[Individual, Individual]
CrossoverOperator = Callable[
    [_Parents, ProblemData, PenaltyManager, XorShift128], Individual
]


@dataclass
class GeneticAlgorithmParams:
    nb_penalty_management: int = 47
    repair_probability: float = 0.80
    collect_statistics: bool = False
    should_intensify: bool = True

    def __post_init__(self):
        if self.nb_penalty_management < 0:
            raise ValueError("nb_penalty_management < 0 not understood.")

        if not 0 <= self.repair_probability <= 1:
            raise ValueError("repair_probability must be in [0, 1].")


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

        self._load_feas: List[bool] = []
        self._tw_feas: List[bool] = []

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

        while not stop(self._pop.get_best_found()):
            iters += 1

            parents = self._pop.select()
            offspring = self._op(parents, self._data, self._pm, self._rng)
            self._educate(offspring)

            if iters % self._params.nb_penalty_management == 0:
                self._update_penalties()

            if self._params.collect_statistics:
                stats.collect_from(self._pop)

        end = time.perf_counter() - start
        return Result(self._pop.get_best_found(), stats, iters, end)

    def _educate(self, individual: Individual):
        self._ls.search(individual)

        # Only intensify feasible, new best solutions. See also the repair
        # step below.
        if (
            self._params.should_intensify
            and individual.is_feasible()
            and individual.cost() < self._pop.get_best_found().cost()
        ):
            self._ls.intensify(individual)

        self._pop.add(individual)
        self._load_feas.append(not individual.has_excess_capacity())
        self._tw_feas.append(not individual.has_time_warp())

        # Possibly repair if current solution is infeasible. In that case, we
        # penalise infeasibility more using a penalty booster.
        if (
            not individual.is_feasible()
            and self._rng.rand() < self._params.repair_probability
        ):
            best_found = self._pop.get_best_found()

            with self._pm.get_penalty_booster() as booster:  # noqa
                self._ls.search(individual)

                if individual.is_feasible():
                    if (
                        self._params.should_intensify
                        and individual.cost() < best_found.cost()
                    ):
                        self._ls.intensify(individual)

                    self._pop.add(individual)

                    # We already know the individual is feasible, so load and
                    # time constraints are both satisfied.
                    self._load_feas.append(True)
                    self._tw_feas.append(True)

    def _update_penalties(self):
        feas_load_pct = sum(self._load_feas) / len(self._load_feas)
        self._pm.update_capacity_penalty(feas_load_pct)
        self._load_feas = []

        feas_tw_pct = sum(self._tw_feas) / len(self._tw_feas)
        self._pm.update_time_warp_penalty(feas_tw_pct)
        self._tw_feas = []
