import time
from typing import List

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


class GeneticAlgorithm:
    class Params:
        nb_penalty_management = 47
        repair_probability = 79
        collect_statistics = False
        should_intensify = True

        # TODO parameter validation

    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        population: Population,
        local_search: LocalSearch,
        crossover_op,
        params: Params = Params(),
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

        if (
            not individual.is_feasible()
            and self._rng.randint(100) < self._params.repair_probability
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
                    self._load_feas.append(
                        not individual.has_excess_capacity()
                    )
                    self._tw_feas.append(not individual.has_time_warp())

    def _update_penalties(self):
        feas_load_pct = sum(self._load_feas) / len(self._load_feas)
        self._pm.update_capacity_penalty(feas_load_pct)
        self._load_feas = []

        feas_tw_pct = sum(self._tw_feas) / len(self._tw_feas)
        self._pm.update_time_warp_penalty(feas_tw_pct)
        self._tw_feas = []


# #include "GeneticAlgorithm.h"
#
# #include "Individual.h"
# #include "LocalSearch.h"
# #include "Population.h"
# #include "ProblemData.h"
# #include "Result.h"
# #include "Statistics.h"
#
# #include <chrono>
# #include <numeric>
# #include <utility>
#
# Result GeneticAlgorithm::run(StoppingCriterion &stop)
# {
#     using clock = std::chrono::system_clock;
#
#     Statistics stats;
#     size_t iter = 0;
#
#     if (data.numClients() <= 1)
#         return {population.getBestFound(), stats, iter, 0.};
#
#     auto start = clock::now();
#     while (not stop(population.getBestFound().cost()))
#     {
#         iter++;
#
#         auto const parents = population.select();
#         auto offspring = crossover(parents, data, penaltyManager, rng);
#         educate(offspring);
#
#         if (iter % params.nbPenaltyManagement == 0)
#             updatePenalties();
#
#         if (params.collectStatistics)
#             stats.collectFrom(population);
#     }
#
#     std::chrono::duration<double> runTime = clock::now() - start;
#     return {population.getBestFound(), stats, iter, runTime.count()};
# }
#
# void GeneticAlgorithm::educate(Individual &indiv)
# {
#     localSearch.search(indiv);
#
#     if (params.shouldIntensify  // only intensify feasible, new best
#         && indiv.isFeasible()   // solutions. Cf. also repair below.
#         && indiv.cost() < population.getBestFound().cost())
#         localSearch.intensify(indiv);
#
#     population.add(indiv);
#
#     loadFeas.push_back(!indiv.hasExcessCapacity());
#     timeFeas.push_back(!indiv.hasTimeWarp());
#
#     // Possibly repair if current solution is infeasible. In that case, we
#     // penalise infeasibility more using a penalty booster.
#     if (!indiv.isFeasible() && rng.randint(100) < params.repairProbability)
#     {
#         auto const booster = penaltyManager.getPenaltyBooster();
#         localSearch.search(indiv);
#
#         if (indiv.isFeasible())
#         {
#             if (params.shouldIntensify
#                 && indiv.cost() < population.getBestFound().cost())
#                 localSearch.intensify(indiv);
#
#             population.add(indiv);
#
#             loadFeas.push_back(!indiv.hasExcessCapacity());
#             timeFeas.push_back(!indiv.hasTimeWarp());
#         }
#     }
# }
#
# void GeneticAlgorithm::updatePenalties()
# {
#     double feasLoadPct = std::reduce(loadFeas.begin(), loadFeas.end());
#     feasLoadPct /= static_cast<double>(loadFeas.size());
#
#     penaltyManager.updateCapacityPenalty(feasLoadPct);
#     loadFeas.clear();
#
#     double feasTimePct = std::reduce(timeFeas.begin(), timeFeas.end());
#     feasTimePct /= static_cast<double>(timeFeas.size());
#
#     penaltyManager.updateTimeWarpPenalty(feasTimePct);
#     timeFeas.clear();
# }
#
# GeneticAlgorithm::GeneticAlgorithm(ProblemData &data,
#                                    PenaltyManager &penaltyManager,
#                                    XorShift128 &rng,
#                                    Population &population,
#                                    LocalSearch &localSearch,
#                                    CrossoverOperator op,
#                                    GeneticAlgorithmParams params)
#     : data(data),
#       penaltyManager(penaltyManager),
#       rng(rng),
#       population(population),
#       localSearch(localSearch),
#       crossover(std::move(op)),
#       params(params)
# {
#     loadFeas.reserve(params.nbPenaltyManagement);
#     timeFeas.reserve(params.nbPenaltyManagement);
# }
