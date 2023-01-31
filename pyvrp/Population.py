from typing import Callable, List, Tuple

from pyvrp._lib.hgspy import (
    Individual,
    PenaltyManager,
    ProblemData,
    XorShift128,
)

_Fitness = float
_SubPop = List[Tuple[Individual, _Fitness]]
_DiversityMeasure = Callable[[ProblemData, Individual, Individual], float]


class Population:
    class Params:
        min_pop_size = 25
        generation_size = 40
        nb_elite = 4
        nb_close = 5
        lb_diversity = 0.1
        ub_diversity = 0.5

        # TODO parameter validation

        @property
        def max_pop_size(self) -> int:
            return self.min_pop_size + self.generation_size

    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        diversity_op: _DiversityMeasure,
        params: Params = Params(),
    ):
        """
        Creates a Population instance.

        Parameters
        ----------
        data
            Data object describing the problem to be solved.
        penalty_manager
            Penalty manager to use.
        rng
            Random number generator.
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._op = diversity_op
        self._params = params

        self._feas: _SubPop = []
        self._infeas: _SubPop = []

        self._best = Individual(data, penalty_manager, rng)

        for _ in range(params.min_pop_size):
            self.add(Individual(data, penalty_manager, rng))

    def add(self, individual: Individual):
        """
        Adds the given individual to the population. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the population.
        """
        is_feasible = individual.is_feasible()
        cost = individual.cost()

        pop = self._feas if is_feasible else self._infeas

        for other, _ in pop:
            pass  # TODO register

        pop.append((individual, 0.0))
        self.update_fitness(pop)

        if len(pop) > self._params.max_pop_size:
            self.purge(pop)

        if is_feasible and cost < self.get_best_found().cost():
            self._best = individual

    def select(self) -> Tuple[Individual, Individual]:
        """
        Selects two (if possible non-identical) parents by binary tournament,
        subject to a diversity restriction.

        Returns
        -------
        tuple
            A pair of individuals (parents).
        """
        first = self.get_binary_tournament()
        second = self.get_binary_tournament()

        diversity = self._op(self._data, first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament()
            diversity = self._op(self._data, first, second)

        return first, second

    def purge(self, pop: _SubPop):
        """
        Performs survivor selection: individuals in the given sub-population
        are purged until the population is reduced to the ``minPopSize``.
        Purging happens first to duplicate solutions, and then to solutions
        with high biased fitness.

        Parameters
        ----------
        pop
            Sub-population to purge.
        """
        # TODO remove duplicates and bad fitness
        del pop[self._params.min_pop_size :]

    def update_fitness(self, pop: _SubPop):
        pass

    def get_binary_tournament(self) -> Individual:
        """
        Select an individual from this population by binary tournament.

        Returns
        -------
        Individual
            The selected individual.
        """

        def select():
            num_feas = len(self._feas)
            num_infeas = len(self._infeas)
            idx = self._rng.randint(num_feas + num_infeas)

            if idx < len(self._feas):
                return self._feas[idx]

            return self._infeas[idx - num_feas]

        indiv1, fitness1 = select()
        indiv2, fitness2 = select()

        return indiv1 if fitness1 < fitness2 else indiv2

    def get_best_found(self) -> Individual:
        """
        Returns the best found solution so far. In early iterations, this
        solution might not be feasible yet.

        Returns
        -------
        Individual
            The best solution found so far.
        """
        return self._best


# #include "Population.h"
#
# #include "Individual.h"
# #include "ProblemData.h"
#
# #include <memory>
# #include <vector>
#
# void Population::add(Individual const &indiv)
# {
#     auto &subPop = indiv.isFeasible() ? feasible : infeasible;
#     auto indivPtr = std::make_unique<Individual>(indiv);
#
#     for (auto const &other : subPop)  // update distance to other individuals
#         registerNearbyIndividual(indivPtr.get(), other.indiv.get());
#
#     subPop.push_back({std::move(indivPtr), 0});
#     updateBiasedFitness(subPop);
#
#     if (subPop.size() > params.minPopSize + params.generationSize)
#         purge(subPop);  // survivor selection
#
#     if (indiv.isFeasible() && indiv.cost() < bestSol.cost())
#         bestSol = indiv;
# }
#
# void Population::updateBiasedFitness(SubPopulation &subPop) const
# {
#     // Sort population by ascending cost
#     std::sort(subPop.begin(), subPop.end(), [](auto &a, auto &b) {
#         return a.indiv->cost() < b.indiv->cost();
#     });
#
#     // Ranking the individuals based on their diversity contribution (
#     decreasing
#     // order of broken pairs distance)
#     std::vector<std::pair<double, size_t>> diversity;
#     for (size_t rank = 0; rank != subPop.size(); rank++)
#     {
#         auto dist = avgDistanceClosest(*subPop[rank].indiv.get());
#         diversity.emplace_back(dist, rank);
#     }
#
#     std::sort(diversity.begin(), diversity.end(), std::greater<>());
#
#     auto const numElite = std::min(params.nbElite, subPop.size());
#
#     for (size_t divRank = 0; divRank != subPop.size(); divRank++)
#     {
#         auto const popSize = static_cast<double>(subPop.size());
#
#         // Ranking the individuals based on the cost and diversity rank
#         auto const costRank = diversity[divRank].second;
#         auto const divWeight = 1 - numElite / popSize;
#
#         subPop[costRank].fitness = (costRank + divWeight * divRank) /
#         popSize;
#     }
# }
#
# void Population::purge(std::vector<IndividualWrapper> &subPop)
# {
#     auto remove = [&](auto &iterator) {
#         auto const *indiv = iterator->indiv.get();
#
#         for (auto [_, individuals] : proximity)
#             for (size_t idx = 0; idx != individuals.size(); ++idx)
#                 if (individuals[idx].second == indiv)
#                 {
#                     individuals.erase(individuals.begin() + idx);
#                     break;
#                 }
#
#         proximity.erase(indiv);
#         subPop.erase(iterator);
#     };
#
#     while (subPop.size() > params.minPopSize)
#     {
#         // Remove duplicates from the subpopulation (if they exist)
#         auto const pred = [&](auto &it) {
#             return proximity.contains(it.indiv.get())
#                    && !proximity.at(it.indiv.get()).empty()
#                    && proximity.at(it.indiv.get()).begin()->first == 0;
#         };
#
#         auto const duplicate = std::find_if(subPop.begin(), subPop.end(),
#         pred);
#
#         if (duplicate == subPop.end())  // there are no more duplicates
#             break;
#
#         remove(duplicate);
#     }
#
#     while (subPop.size() > params.minPopSize)
#     {
#         // Remove the worst individual (worst in terms of biased fitness)
#         updateBiasedFitness(subPop);
#
#         auto const worstFitness = std::max_element(
#             subPop.begin(), subPop.end(), [](auto const &a, auto const &b) {
#                 return a.fitness < b.fitness;
#             });
#
#         remove(worstFitness);
#     }
# }
#
# void Population::registerNearbyIndividual(Individual *first, Individual
# *second)
# {
#     auto const dist = divOp(data, *first, *second);
#     auto cmp = [](auto &elem, auto &value) { return elem.first < value; };
#
#     auto &fProx = proximity[first];
#     auto place = std::lower_bound(fProx.begin(), fProx.end(), dist, cmp);
#     fProx.emplace(place, dist, second);
#
#     auto &sProx = proximity[second];
#     place = std::lower_bound(sProx.begin(), sProx.end(), dist, cmp);
#     sProx.emplace(place, dist, first);
# }
#
# double Population::avgDistanceClosest(Individual const &indiv) const
# {
#     if (!proximity.contains(&indiv) || proximity.at(&indiv).empty())
#         return 0.;
#
#     auto const &prox = proximity.at(&indiv);
#     auto const maxSize = std::min(params.nbClose, prox.size());
#     auto start = prox.begin();
#     int result = 0;
#
#     for (auto it = start; it != start + maxSize; ++it)
#         result += it->first;
#
#     return result / static_cast<double>(maxSize);
# }
#
# Individual const *Population::getBinaryTournament()
# {
#     auto const fSize = numFeasible();
#
#     auto const idx1 = rng.randint(size());
#     auto &wrap1 = idx1 < fSize ? feasible[idx1] : infeasible[idx1 - fSize];
#
#     auto const idx2 = rng.randint(size());
#     auto &wrap2 = idx2 < fSize ? feasible[idx2] : infeasible[idx2 - fSize];
#
#     return (wrap1.fitness < wrap2.fitness ? wrap1.indiv : wrap2.indiv).get();
# }
#
# std::pair<Individual const *, Individual const *> Population::select()
# {
#     auto const *par1 = getBinaryTournament();
#     auto const *par2 = getBinaryTournament();
#
#     auto diversity = divOp(data, *par1, *par2);
#
#     size_t tries = 1;
#     while ((diversity < params.lbDiversity || diversity > params.ubDiversity)
#            && tries++ < 10)
#     {
#         par2 = getBinaryTournament();
#         diversity = divOp(data, *par1, *par2);
#     }
#
#     return std::make_pair(par1, par2);
# }
#
# size_t Population::size() const { return numFeasible() + numInfeasible(); }
#
# size_t Population::numFeasible() const { return feasible.size(); }
#
# size_t Population::numInfeasible() const { return infeasible.size(); }
#
# Individual const &Population::getBestFound() const { return bestSol; }
#
# Population::Population(ProblemData const &data,
#                        PenaltyManager const &penaltyManager,
#                        XorShift128 &rng,
#                        DiversityMeasure op,
#                        PopulationParams params)
#     : data(data),
#       rng(rng),
#       divOp(std::move(op)),
#       params(params),
#       bestSol(data, penaltyManager, rng)  // random initial best solution
# {
#     // Generate minPopSize random individuals to seed the population.
#     for (size_t count = 0; count != params.minPopSize; ++count)
#         add({data, penaltyManager, rng});
# }
