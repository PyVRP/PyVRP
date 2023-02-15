from __future__ import annotations

from bisect import insort_left
from dataclasses import dataclass
from typing import (
    Callable,
    Generic,
    Iterator,
    List,
    NamedTuple,
    Optional,
    Protocol,
    Tuple,
    Type,
    TypeVar,
    Union,
    cast,
)

from pyvrp.diversity.BiasedFitness import BiasedFitness

from .Individual import Individual
from .PenaltyManager import PenaltyManager
from .ProblemData import ProblemData
from .XorShift128 import XorShift128


class PopulationIndividual(Protocol):  # pragma: no cover
    """
    Defines an individual that can be put in a population target protocol.
    """

    def cost(self) -> Union[int, float]:
        ...

    def is_feasible(self) -> bool:
        ...


TIndiv = TypeVar("TIndiv", bound=PopulationIndividual)

_DiversityMeasure = Callable[
    [PopulationIndividual, PopulationIndividual], float
]
_RandIntFunc = Callable[[int], int]


class _DiversityItem(NamedTuple):
    individual: PopulationIndividual
    diversity: float

    def __lt__(self, other) -> bool:
        # Note: this is only used for computing most similar indivs for
        # average diversity, therefore a tie-breaker on cost is not necessary
        return (
            isinstance(other, _DiversityItem)
            and self.diversity < other.diversity
        )


class _Item(NamedTuple):
    individual: PopulationIndividual
    fitness: float
    cost: float


class _ItemWithProximity(NamedTuple):
    item: _Item
    proximity: List[_DiversityItem]


class SubPopulation(Generic[TIndiv]):
    def __init__(
        self,
        diversity_op: _DiversityMeasure,
        params: PopulationParams,
    ):
        self._op = diversity_op

    def __getitem__(self, idx: int) -> TIndiv:
        raise NotImplementedError()

    def __iter__(self) -> Iterator[TIndiv]:
        raise NotImplementedError()

    def __len__(self) -> int:
        raise NotImplementedError()

    def add(self, individual: TIndiv):
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        """
        raise NotImplementedError()

    def avg_distance_closest(self, individual_idx: int) -> float:
        raise NotImplementedError()

    def get_biased_fitness(self, individual_idx: int) -> float:
        raise NotImplementedError()


class SubPopulationPython(SubPopulation, Generic[TIndiv]):
    def __init__(
        self,
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
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._op = diversity_op
        self._params = params

        self._items: List[_ItemWithProximity] = []

    def __getitem__(self, idx: int) -> TIndiv:
        return cast(TIndiv, self._items[idx].item.individual)

    def __iter__(self) -> Iterator[TIndiv]:
        for item in self._items:
            yield cast(TIndiv, item.item.individual)

    def __len__(self) -> int:
        return len(self._items)

    def add(self, individual: PopulationIndividual):
        indiv_prox: List[_DiversityItem] = []

        for (other, _, _), other_prox in self._items:
            diversity = self._op(individual, other)
            insort_left(indiv_prox, _DiversityItem(other, diversity))
            insort_left(other_prox, _DiversityItem(individual, diversity))

        # Insert new individual and update everyone's biased fitness score.
        self._items.append(
            _ItemWithProximity(
                _Item(individual, 0.0, individual.cost()), indiv_prox
            )
        )
        self._items.sort(key=lambda it: it.item.cost)
        self.update_fitness()

        if len(self) >= self._params.max_pop_size:
            self.purge()

    def remove(self, individual: PopulationIndividual):
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
        for _, prox in self._items:  # remove individual from other proximities
            for idx, (other, _) in enumerate(prox):
                if other is individual:
                    del prox[idx]
                    break

        for item in self._items:  # remove individual from subpopulation.
            if item.item.individual is individual:
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
            for (individual, _, _), prox in self._items:
                if prox and prox[0].individual == individual:  # has duplicate?
                    self.remove(individual)
                    break
            else:  # no duplicates, so break loop
                break

        while len(self) > self._params.min_pop_size:
            self.update_fitness()

            # Find the worst individual (in terms of biased fitness) in the
            # population, and remove it.
            self.remove(
                max(
                    self._items, key=lambda item: item.item.fitness
                ).item.individual
            )

    def update_fitness(self):
        """
        Updates the biased fitness scores of individuals in the subpopulation.
        This fitness depends on the quality of the solution (based on its cost)
        and the diversity w.r.t. to other individuals in the subpopulation.
        """
        # Sort first by largest diversity, then smallest cost as tie-breaker
        diversity = sorted(
            range(len(self)),
            key=lambda idx: (-self.avg_distance_closest(idx), idx),
        )

        nb_elite = min(self._params.nb_elite, len(self))
        div_weight = 1 - nb_elite / len(self)

        for div_rank, cost_rank in enumerate(diversity):
            new_fitness = (cost_rank + div_weight * div_rank) / len(self)
            (individual, _, cost), prox = self._items[cost_rank]
            self._items[cost_rank] = _ItemWithProximity(
                _Item(individual, new_fitness, cost), prox
            )

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
        item = self._items[individual_idx]
        closest = item.proximity[: self._params.nb_close]

        if closest:
            return sum(div for _, div in closest) / len(closest)
        else:
            return 0.0

    def get_biased_fitness(self, individual_idx: int) -> float:
        return self._items[individual_idx].item.fitness


class SubPopulationNumpy(SubPopulation, Generic[TIndiv]):
    def __init__(
        self,
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
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._op = diversity_op
        self._params = params
        self._biased_fitness = BiasedFitness(
            params.nb_close, params.max_pop_size, params.nb_elite
        )

        self._indivs: List[TIndiv] = []

    def __getitem__(self, idx: int) -> TIndiv:
        return self._indivs[idx]

    def __iter__(self) -> Iterator[TIndiv]:
        return iter(self._indivs)

    def __len__(self) -> int:
        return len(self._indivs)

    def add(self, individual: TIndiv):
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        """
        self._biased_fitness.add(
            individual.cost(), [self._op(individual, other) for other in self]
        )
        self._indivs.append(individual)

        if len(self) >= self._params.max_pop_size:
            self.purge()

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
        return self._biased_fitness.avg_distance_closest(individual_idx)

    def get_biased_fitness(self, individual_idx: int) -> float:
        return self._biased_fitness[individual_idx]

    def remove(self, individual: PopulationIndividual):
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
        raise NotImplementedError()

    def purge(self):
        """
        Performs survivor selection: individuals in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.
        """
        idx_survivor = self._biased_fitness.purge(self._params.min_pop_size)
        self._indivs = [self._indivs[i] for i in idx_survivor]


@dataclass
class PopulationParams:
    min_pop_size: int = 25
    generation_size: int = 40
    nb_elite: int = 4
    nb_close: int = 5
    lb_diversity: float = 0.1
    ub_diversity: float = 0.5
    use_numpy: bool = True

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


class GenericPopulation(Generic[TIndiv]):
    def __init__(
        self,
        best: TIndiv,
        diversity_op: Callable[[TIndiv, TIndiv], float],
        rand_int_func: _RandIntFunc,
        params: PopulationParams = PopulationParams(),
    ):
        """
        Creates a GenericPopulation instance.

        Parameters
        ----------
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        rand_int_func
            Function to generate random integer
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._op = cast(_DiversityMeasure, diversity_op)
        self._rand_int_func = rand_int_func
        self._params = params

        subpop_cls: Type[SubPopulation] = (
            SubPopulationNumpy if params.use_numpy else SubPopulationPython
        )
        self._feas = subpop_cls(self._op, params)
        self._infeas = subpop_cls(self._op, params)

        self._best = best

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

    def initialize(self, factory: Callable[[], TIndiv]):
        for _ in range(self._params.min_pop_size):
            self.add(factory())

    def add(self, individual: TIndiv):
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

    def select(self) -> Tuple[TIndiv, TIndiv]:
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

        diversity = self._op(first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament()
            diversity = self._op(first, second)

        return first, second

    def get_binary_tournament(self) -> TIndiv:
        """
        Selects an individual from this population by binary tournament.

        Returns
        -------
        Individual
            The selected individual.
        """

        def select():
            num_feas = len(self._feas)
            idx = self._rand_int_func(len(self))

            if idx < num_feas:
                return self._feas[idx], self._feas.get_biased_fitness(idx)

            return self._infeas[
                idx - num_feas
            ], self._infeas.get_biased_fitness(idx - num_feas)

        indiv1, fitness1 = select()
        indiv2, fitness2 = select()

        if fitness1 < fitness2:
            return indiv1

        return indiv2

    def get_best_found(self) -> TIndiv:
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


_IndividualDiversityMeasure = Callable[
    [ProblemData, Individual, Individual], float
]


class Population(GenericPopulation[Individual]):
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        diversity_op: _IndividualDiversityMeasure,
        initial_solutions: Optional[List[Individual]] = None,
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
        initial_solutions
            List of Individuals to populate the population with.
            Will be generated randomly if None.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        super().__init__(
            Individual(data, penalty_manager, rng),
            lambda first, second: diversity_op(data, first, second),
            rng.randint,
            params,
        )

        if initial_solutions:
            for indiv in initial_solutions:
                self.add(indiv)
        else:
            self.initialize(lambda: Individual(data, penalty_manager, rng))
