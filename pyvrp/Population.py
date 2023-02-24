from __future__ import annotations

from dataclasses import dataclass
from statistics import fmean
from typing import Callable, Iterator, List, NamedTuple, Tuple

import numpy as np

from ._Individual import Individual
from ._PenaltyManager import PenaltyManager
from ._ProblemData import ProblemData
from ._XorShift128 import XorShift128

_DiversityMeasure = Callable[[ProblemData, Individual, Individual], float]


class _DiversityItem(NamedTuple):
    individual: Individual
    diversity: float


class _Item(NamedTuple):
    individual: Individual
    fitness: float
    proximity: List[_DiversityItem]


class SubPopulation:
    def __init__(
        self,
        data: ProblemData,
        diversity_op: _DiversityMeasure,
        params: PopulationParams,
    ):
        """
        Creates a SubPopulation instance.

        This subpopulation manages a number individuals, and initiates survivor
        selection (purging) when their number grows large. A subpopulation's
        individuals (and metadata) can be accessed via indexing and iteration.
        Each individual is stored as a tuple of type ``_Item``, which stores
        the individual itself, a fitness score (higher is worse), and a list
        of proximity values to the other individuals in the subpopulation.

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

    def __getitem__(self, idx: int) -> _Item:
        return self._items[idx]

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
        indiv_prox = []

        for other, _, other_prox in self:
            diversity = self._op(self._data, individual, other)

            # TODO this is not very fast, so we might want to go for something
            #  different if this turns out to be problematic.
            indiv_prox.append(_DiversityItem(other, diversity))
            indiv_prox.sort(key=lambda a: (a.diversity, a.individual.cost()))

            other_prox.append(_DiversityItem(individual, diversity))
            other_prox.sort(key=lambda a: (a.diversity, a.individual.cost()))

        # Insert new individual and update everyone's biased fitness score.
        self._items.append(_Item(individual, 0.0, indiv_prox))
        self.update_fitness()

        if len(self) > self._params.max_pop_size:
            self.purge()

    def remove(self, individual: Individual):
        """
        Removes the given individual from the subpopulation. Note that
        individuals are compared by identity, not by equality.

        Parameters
        ----------
        individual
            The individual to remove.

        Raises
        ------
        ValueError
            When the given individual could not be found in the subpopulation.
        """
        for _, _, prox in self:  # remove individual from other proximities
            for idx, (other, _) in enumerate(prox):
                if other is individual:
                    del prox[idx]
                    break

        for item in self:  # remove individual from subpopulation.
            if item.individual is individual:
                self._items.remove(item)
                break
        else:
            raise ValueError(f"Individual {individual} not in subpopulation!")

    def purge(self):
        """
        Performs survivor selection: individuals in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.
        """

        while len(self) > self._params.min_pop_size:
            for individual, _, prox in self:
                if prox and prox[0].individual == individual:  # has duplicate?
                    self.remove(individual)
                    break
            else:  # no duplicates, so break loop
                break

        while len(self) > self._params.min_pop_size:
            self.update_fitness()

            # Find the worst individual (in terms of biased fitness) in the
            # population, and remove it.
            self.remove(max(self, key=lambda item: item.fitness).individual)

    def update_fitness(self):
        """
        Updates the biased fitness scores of individuals in the subpopulation.
        This fitness depends on the quality of the solution (based on its cost)
        and the diversity w.r.t. to other individuals in the subpopulation.
        """
        by_cost = np.argsort([indiv.cost() for indiv, _, _ in self])
        diversity = []

        for rank in range(len(self)):
            avg_diversity = self.avg_distance_closest(by_cost[rank])
            diversity.append((avg_diversity, rank))

        diversity.sort(reverse=True)
        nb_elite = min(self._params.nb_elite, len(self))

        for div_rank, (_, cost_rank) in enumerate(diversity):
            div_weight = 1 - nb_elite / len(self)
            new_fitness = (cost_rank + div_weight * div_rank) / len(self)

            idx = by_cost[cost_rank]
            individual, _, prox = self[idx]
            self._items[idx] = _Item(individual, new_fitness, prox)

    def avg_distance_closest(self, individual_idx: int) -> float:
        """
        Determines the average distance of the individual at the given index to
        a number of individuals that are most similar to it. This provides a
        measure of the relative 'diversity' of this individual.

        Parameters
        ----------
        individual_idx
            Individual whose average distance/diversity to calculate.

        Returns
        -------
        float
            The average distance/diversity of the given individual relative to
            the total subpopulation.
        """
        _, _, prox = self[individual_idx]
        closest = prox[: self._params.nb_close]
        return fmean(div for _, div in closest) if closest else 0.0


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

    def restart(self):
        """
        Restarts the population. All individuals are removed and a new initial
        population population is generated.
        """
        self._feas = SubPopulation(self._data, self._op, self._params)
        self._infeas = SubPopulation(self._data, self._op, self._params)

        for _ in range(self._params.min_pop_size):
            self.add(Individual(self._data, self._pm, self._rng))

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

        item1 = select()
        item2 = select()

        if item1.fitness < item2.fitness:
            return item1.individual

        return item2.individual
