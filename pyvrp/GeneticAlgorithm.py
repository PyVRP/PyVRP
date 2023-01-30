import time
from typing import List

from pyvrp.Result import Result
from pyvrp._lib.hgspy import (
    GeneticAlgorithmParams,
    Individual,
    LocalSearch,
    PenaltyManager,
    Population,
    ProblemData,
    Statistics,
    XorShift128,
)
from pyvrp.stop import StoppingCriterion


class GeneticAlgorithm:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        population: Population,
        local_search: LocalSearch,
        crossover_op,
        params: GeneticAlgorithmParams = GeneticAlgorithmParams(),
    ):
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._pop = population
        self._ls = local_search
        self._op = crossover_op
        self._params = params

        self._best = Individual(data, penalty_manager, rng)  # init rand sol

        self._load_feas: List[bool] = []
        self._tw_feas: List[bool] = []

    def run(self, stop: StoppingCriterion):
        start = time.perf_counter()
        stats = Statistics()
        iters = 0

        while not stop(self._best):
            iters += 1

            parents = self._pop.select()
            offspring = self._op(parents, self._data, self._pm, self._rng)
            self._educate(offspring)

            if iters % self._params.nb_penalty_management == 0:
                self._update_penalties()

            if self._params.collect_statistics:
                stats.collect_from(self._pop)

        end = time.perf_counter() - start
        return Result(self._best, stats, iters, end)

    def _educate(self, individual: Individual):
        self._ls.search(individual)
        is_new_best = individual.cost() < self._best.cost()

        # Only intensify feasible, new best solutions. See also the repair
        # step below.
        if (
            self._params.should_intensify
            and individual.is_feasible()
            and is_new_best
        ):
            self._ls.intensify(individual)

        if is_new_best:
            self._best = individual

        self._pop.add(individual)
        self._load_feas.append(not individual.has_excess_capacity())
        self._tw_feas.append(not individual.has_time_warp())

        if (
            not individual.is_feasible()
            and self._rng.randint(100) < self._params.repair_probability
        ):
            # TODO booster

            self._ls.search(individual)
            is_new_best = individual.cost() < self._best.cost()

            if individual.is_feasible():
                if self._params.should_intensify and is_new_best:
                    self._ls.intensify(individual)

                if is_new_best:
                    self._best = individual

                self._pop.add(individual)
                self._load_feas.append(not individual.has_excess_capacity())
                self._tw_feas.append(not individual.has_time_warp())

    def _update_penalties(self):
        feas_load_pct = sum(self._load_feas) / len(self._load_feas)
        self._pm.update_capacity_penalty(feas_load_pct)
        self._load_feas = []

        feas_tw_pct = sum(self._tw_feas) / len(self._tw_feas)
        self._pm.update_time_warp_penalty(feas_tw_pct)
        self._tw_feas = []
