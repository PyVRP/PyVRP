from __future__ import annotations

import bisect
from dataclasses import dataclass
from typing import Callable, Dict, Iterator, List, Tuple

import numpy as np

from pyvrp._lib.hgspy import (
    Individual,
    PenaltyManager,
    ProblemData,
    XorShift128,
)


@dataclass
class _DiversityWrapper:
    individual: Individual
    diversity: float

    def __lt__(self, other: _DiversityWrapper) -> bool:
        if not isinstance(other, _DiversityWrapper):
            return False

        return self.diversity < other.diversity or (
            self.diversity == other.diversity
            and self.individual.cost() < other.individual.cost()
        )

    def __iter__(self):  # for unpacking
        return iter((self.individual, self.diversity))


_DiversityMeasure = Callable[[ProblemData, Individual, Individual], float]
_ProxType = Dict[Individual, List[_DiversityWrapper]]
_Item = Tuple[Individual, float]  # (individual, fitness score)


class SubPopulation:
    def __init__(
        self,
        data: ProblemData,
        diversity_op: _DiversityMeasure,
        params: PopulationParams,
    ):
        """
        Creates a SubPopulation instances. This subpopulation manages a number
        individuals, tracking their diversities, and initiating survivor
        selection (purging) when their number grows large.

        Parameters
        ----------
        data
            Data object describing the problem to be solved.
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._data = data
        self._op = diversity_op
        self._params = params

        self._items: List[_Item] = []
        self._prox: _ProxType = {}

    @property
    def proximity_structure(self) -> _ProxType:
        """
        Returns the proximity structure maintained by this subpopulation. The
        proximity structure is used for diversity and fitness calculations.

        Returns
        -------
        _ProxType
            Proximity structure.
        """
        return self._prox

    def __getitem__(self, idx: int) -> _Item:
        return self._items[idx]

    def __setitem__(self, idx: int, value: _Item):
        self._items[idx] = value

    def __iter__(self) -> Iterator[_Item]:
        return iter(self._items)

    def __len__(self) -> int:
        return len(self._items)

    def add(self, individual: Individual):
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        """
        self._prox[individual] = []

        for other, _ in self:
            dist = self._op(self._data, individual, other)

            other_wrapper = _DiversityWrapper(other, dist)
            bisect.insort_left(self._prox[individual], other_wrapper)

            indiv_wrapper = _DiversityWrapper(individual, dist)
            bisect.insort_left(self._prox[other], indiv_wrapper)

        self._items.append((individual, 0.0))  # fitness is updated next.
        self.update_fitness()

        if len(self) > self._params.max_pop_size:
            self.purge()

    def remove(self, individual: Individual):
        # TODO some of these equality comparisons could probably also be done
        #  via id(), that is, the memory locations the objects occupy.
        for _, others in self._prox.items():
            for idx, (other, _) in enumerate(others):
                if other == individual:
                    del others[idx]
                    break

        del self._prox[individual]

        for other in self:
            if other[0] == individual:
                self._items.remove(other)
                break
        else:
            # This else should never happen, because the proximity and items
            # structure should be synched, and then the ``del`` statement above
            # would have raised before.
            raise ValueError(f"Individual {individual} not in subpopulation!")

    def purge(self):
        """
        Performs survivor selection: individuals in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.
        """

        while len(self) > self._params.min_pop_size:
            for individual, _ in self:
                prox = self._prox[individual]

                if prox and np.isclose(prox[0].diversity, 0.0):
                    self.remove(individual)  # individual has a duplicate
                    break
            else:  # no duplicates, so break loop
                break

        while len(self) > self._params.min_pop_size:
            self.update_fitness()

            # Find the individual with worst (highest) biased fitness, and
            # remove it from the subpopulation.
            worst_individual, _ = max(self, key=lambda item: item[1])
            self.remove(worst_individual)

    def update_fitness(self):
        """
        Updates the biased fitness scores of individuals in the subpopulation.
        This fitness depends on the quality of the solution (based on its cost)
        and the diversity w.r.t. to other individuals in the subpopulation.
        """
        by_cost = np.argsort([indiv.cost() for indiv, _ in self])
        diversity = []

        for rank in range(len(self)):
            individual, _ = self[by_cost[rank]]
            avg_diversity = self.avg_distance_closest(individual)
            diversity.append((avg_diversity, rank))

        diversity.sort(reverse=True)
        nb_elite = min(self._params.nb_elite, len(self))

        for div_rank, (_, cost_rank) in enumerate(diversity):
            div_weight = 1 - nb_elite / len(self)
            new_fitness = (cost_rank + div_weight * div_rank) / len(self)

            individual, _ = self[by_cost[cost_rank]]
            self[by_cost[cost_rank]] = individual, new_fitness

    def avg_distance_closest(self, individual: Individual) -> float:
        """
        Determines the average distance of the given individual to a number of
        individuals that are most similar to it. This provides a measure of the
        relative 'diversity' of this individual.

        Parameters
        ----------
        individual
            Individual whose average distance/diversity to calculate.

        Returns
        -------
        float
            The average distance/diversity of the given individual relative to
            the total subpopulation.
        """
        closest = self._prox[individual][: self._params.nb_close]
        return np.mean([div for _, div in closest]) if closest else 0.0


@dataclass
class PopulationParams:
    min_pop_size: int = 25
    generation_size: int = 40
    nb_elite: int = 4
    nb_close: int = 5
    lb_diversity: float = 0.1
    ub_diversity: float = 0.5

    def __post_init__(self):
        if not 0 <= self.lb_diversity <= 1.0:
            raise ValueError("lb_diversity must be in [0, 1].")

        if not 0 <= self.ub_diversity <= 1.0:
            raise ValueError("ub_diversity must be in [0, 1].")

        if self.ub_diversity <= self.lb_diversity:
            raise ValueError("ub_diversity <= lb_diversity not understood.")

        if self.nb_elite < 0:
            raise ValueError("nb_elite < 0 not understood.")

        if self.nb_close < 0:
            raise ValueError("nb_close < 0 not understood.")

        if self.min_pop_size < 0:
            # It'll be pretty difficult to do crossover without individuals,
            # but that should not matter much to the Population. Perhaps the
            # user wants to add the pops by themselves somehow.
            raise ValueError("min_pop_size < 0 not understood")

        if self.generation_size < 0:
            raise ValueError("generation_size < 0 not understood.")

    @property
    def max_pop_size(self) -> int:
        return self.min_pop_size + self.generation_size


class Population:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        diversity_op: _DiversityMeasure,
        params: PopulationParams = PopulationParams(),
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

        self._feas = SubPopulation(data, diversity_op, params)
        self._infeas = SubPopulation(data, diversity_op, params)

        self._best = Individual(data, penalty_manager, rng)

        for _ in range(params.min_pop_size):
            self.add(Individual(data, penalty_manager, rng))

    @property
    def feasible_subpopulation(self) -> SubPopulation:
        """
        Returns the feasible subpopulation maintained by this population
        instance.

        Returns
        -------
        SubPopulation
            Feasible subpopulation.
        """
        return self._feas

    @property
    def infeasible_subpopulation(self) -> SubPopulation:
        """
        Returns the infeasible subpopulation maintained by this population
        instance.

        Returns
        -------
        SubPopulation
            Infeasible subpopulation.
        """
        return self._infeas

    def __len__(self) -> int:
        """
        Returns the current population size, that is, the size of its feasible
        and infeasible subpopulations.

        Returns
        -------
        int
            Population size.
        """
        return len(self._feas) + len(self._infeas)

    def add(self, individual: Individual):
        """
        Adds the given individual to the population. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the population.
        """

        if individual.is_feasible():
            self._feas.add(individual)

            if individual.cost() < self._best.cost():
                self._best = individual
        else:
            self._infeas.add(individual)

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

    def get_binary_tournament(self) -> Individual:
        """
        Selects an individual from this population by binary tournament.

        Returns
        -------
        Individual
            The selected individual.
        """

        def select():
            num_feas = len(self._feas)
            idx = self._rng.randint(len(self))

            if idx < num_feas:
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
        # TODO move this best solution stuff to GeneticAlgorithm
        return self._best
