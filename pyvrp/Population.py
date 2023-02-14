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

import numpy as np

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


class SubPopulation:
    def __init__(
        self,
        diversity_op: _DiversityMeasure,
        params: PopulationParams,
    ):
        self._op = diversity_op

    def __getitem__(self, idx: int) -> _Item:
        raise NotImplementedError()

    def __iter__(self) -> Iterator[_Item]:
        raise NotImplementedError()

    def __len__(self) -> int:
        raise NotImplementedError()

    def add(
        self,
        individual: PopulationIndividual,
        cost: Optional[float] = None,
        distances: Optional[List[float]] = None,
    ):
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        cost, optional
            Cost of individual that is added, if none will be computed
        distances, optional
            List with diversities to individuals currently in the population.
            If none, will be computed.
        """
        if cost is None:
            cost = individual.cost()
        if distances is None:
            distances = [self._op(individual, other) for other, _, _ in self]
        self._add(individual, cost, distances)

    def _add(
        self,
        individual: PopulationIndividual,
        cost: float,
        distances: List[float],
    ):
        raise NotImplementedError()

    def avg_distance_closest(self, individual_idx: int) -> float:
        raise NotImplementedError()


class SubPopulationPython(SubPopulation):
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

    def __getitem__(self, idx: int) -> _Item:
        return self._items[idx].item

    def __iter__(self) -> Iterator[_Item]:
        for item in self._items:
            yield item.item

    def __len__(self) -> int:
        return len(self._items)

    def _add(
        self,
        individual: PopulationIndividual,
        cost: float,
        distances: List[float],
    ):
        indiv_prox: List[_DiversityItem] = []

        for ((other, _, _), other_prox), diversity in zip(
            self._items, distances
        ):
            insort_left(indiv_prox, _DiversityItem(other, diversity))
            insort_left(other_prox, _DiversityItem(individual, diversity))

        # Insert new individual and update everyone's biased fitness score.
        self._items.append(
            _ItemWithProximity(_Item(individual, 0.0, cost), indiv_prox)
        )
        self._items.sort(key=lambda it: it.item.cost)
        self.update_fitness()

        if len(self) > self._params.max_pop_size:
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
            self.remove(max(self, key=lambda item: item.fitness).individual)

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


class SubPopulationNumpy(SubPopulation):
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

        n = params.max_pop_size + 1
        # n x n with distances (differences)
        self._dist = np.zeros((n, n), dtype=float)
        # n x k with indices of closest others
        self._idx_closest = np.zeros((n, params.nb_close), dtype=int)
        # n x k with distances to closest others
        self._dist_closest = np.zeros((n, params.nb_close), dtype=float)
        self._dist_closest_max = np.zeros(n, dtype=float)
        self._dist_closest_argmax = np.zeros(n, dtype=int)
        # Average distance to closest (a.k.a. diversity)
        self._dist_closest_sum = np.zeros(n, dtype=float)
        # self._dist_closest_mean = np.zeros(n, dtype=float)

        # Cost/diversity ranks
        self._cost = np.zeros(n, dtype=float)
        self._cost_rank = np.zeros(n, dtype=int)
        # self._diversity_rank = np.zeros(n, dtype=int)
        self._biased_fitness = np.zeros(n, dtype=float)

        self._indivs: List[PopulationIndividual] = []

        self.DO_CHECKS = False

    def __getitem__(self, idx: int) -> _Item:
        return _Item(
            self._indivs[idx], self._biased_fitness[idx], self._cost[idx]
        )

    def __iter__(self) -> Iterator[_Item]:
        for indiv, fitness, cost in zip(
            self._indivs, self._biased_fitness, self._cost
        ):
            yield _Item(indiv, fitness, cost)

    def __len__(self) -> int:
        return len(self._indivs)

    def _add(
        self,
        individual: PopulationIndividual,
        cost: float,
        distances: List[float],
    ):
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        cost, optional
            Cost of individual that is added, if none will be computed
        distances, optional
            List with diversities to individuals currently in the population.
            If none, will be computed.
        """

        # Get n = current size / index of new item and increase size
        n = len(self)

        if n == 0:
            # # Insert new individual
            self._indivs.append(individual)
            self._cost[n] = cost
            # First individual, no diversity computation needed
            return

        # Update the distance matrix
        dist = np.array(distances)
        self._dist[n, :n] = dist
        self._dist[:n, n] = dist

        if n <= self._params.nb_close:
            # Now we have n + 1 population size so at most n neighbours
            # since n <= k all neighbours can be
            self._idx_closest[:n, n - 1] = n
            self._dist_closest[:n, n - 1] = dist
            self._dist_closest_sum[:n] += dist

            # Find where the newly added is the furthest / least closest
            # and update the max/argmax
            idx_update = np.where(dist > self._dist_closest_max[:n])
            self._dist_closest_max[idx_update] = dist[idx_update]
            self._dist_closest_argmax[idx_update] = n - 1

            # Add closest for newly added
            self._idx_closest[n, :n] = np.arange(n)
            self._dist_closest[n, :n] = dist
            dist_argmax = dist.argmax()
            self._dist_closest_max[n] = dist[dist_argmax]
            self._dist_closest_argmax[n] = dist_argmax
            self._dist_closest_sum[n] = dist.sum()

        else:

            # Find idx of rows that should replace their least closest by
            # the newly added individual with index n
            idx = np.where(dist < self._dist_closest_max[:n])
            # Find position/column to replace (closest are not sorted)
            col = self._dist_closest_argmax[idx]
            self._idx_closest[idx, col] = n
            self._dist_closest[idx, col] = dist[idx]
            # Update sum of closest after replacing maximum
            self._dist_closest_sum[idx] += (
                dist[idx] - self._dist_closest_max[idx]
            )

            # Find position/column of new argmax and update max/argmax
            col = self._dist_closest[idx, :].argmax(-1)
            self._dist_closest_max[idx] = self._dist_closest[idx, col]
            self._dist_closest_argmax[idx] = col

            # Add closest for newly added
            k = self._params.nb_close
            # Use argpartition to find k nearest in linear time
            idx_closest = np.argpartition(dist, k - 1)[:k]
            self._idx_closest[n, :k] = idx_closest
            self._dist_closest[n, :k] = dist[idx_closest]
            self._dist_closest_sum[n] = self._dist_closest[n, :k].sum(-1)
            # Note: argpartition puts the k-th smallest in 0-based index k - 1
            self._dist_closest_max[n] = self._dist_closest[n, k - 1]
            self._dist_closest_argmax[n] = k - 1

        # Compute cost rank and insert individual by increasing rank of worse
        cost_rank = (self._cost[:n] <= cost).sum()
        self._cost_rank[:n][self._cost_rank[:n] >= cost_rank] += 1
        self._cost[n] = cost
        self._cost_rank[n] = cost_rank

        # Compute biased fitness (note, n + 1 includes new individual)
        k = min(self._params.nb_close, n)  # At most n others with n + 1 pop
        diversity = self._dist_closest_sum[: n + 1] / k
        self._biased_fitness[: n + 1] = self._compute_biased_fitness(
            self._cost_rank[: n + 1], diversity
        )

        # # Insert new individual
        self._indivs.append(individual)

        self._check_consistency()

        if len(self) > self._params.max_pop_size:
            self.purge()

    def _check_consistency(self):
        if not self.DO_CHECKS:
            return
        n = len(self)
        k = min(self._params.nb_close, n - 1)
        rng = np.arange(n)

        dist_closest = np.sort(self._dist[:n, :n] + np.eye(n) * 1e9, -1)[:, :k]
        assert np.allclose(
            np.sort(self._dist_closest[:n, :k], -1), dist_closest
        )

        maxv = self._dist_closest[:n, :k].max(-1)
        assert np.allclose(
            self._dist_closest[rng, self._dist_closest_argmax[:n]], maxv
        )
        assert np.allclose(self._dist_closest_max[:n], maxv)
        assert np.allclose(
            self._dist_closest[:n, :k].sum(-1), self._dist_closest_sum[:n]
        )

        diversity = self._dist_closest_sum[:n] / k
        assert np.allclose(diversity, self._dist_closest[:n, :k].mean(-1))

        cost_rank = np.zeros(n, dtype=int)
        argsort_cost = np.argsort(self._cost[:n])
        cost_rank[argsort_cost] = rng
        assert np.allclose(
            self._cost[np.argsort(self._cost_rank[:n])],
            np.sort(self._cost[:n]),
        )
        # Note: below does not work with duplicates
        # assert np.allclose(self._cost_rank[:n], cost_rank)

        check_biased_fitness = self._compute_biased_fitness(
            self._cost_rank[:n], diversity
        )
        # Note, biased fitness may differ by stability
        assert np.allclose(self._biased_fitness[:n], check_biased_fitness)

    def _compute_biased_fitness(
        self, cost_rank: np.ndarray[int], diversity: np.ndarray[float]
    ) -> np.ndarray[float]:
        assert len(cost_rank) == len(diversity)
        n = len(cost_rank)

        if n == 0:
            return np.zeros_like(diversity)

        # Compute diversity rank
        # diversity_argsort = np.argsort(-diversity)
        diversity_argsort = np.lexsort((cost_rank, -diversity))
        diversity_rank = np.zeros(n, dtype=int)
        diversity_rank[diversity_argsort] = np.arange(n)

        nb_elite = min(self._params.nb_elite, n)
        div_weight = 1 - nb_elite / n

        return (cost_rank + div_weight * diversity_rank) / n

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
        k = min(self._params.nb_close, len(self) - 1)
        return self._dist_closest_sum[individual_idx] / k

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

        # Compute at once sorted distance matrix
        n = len(self)
        if n <= self._params.min_pop_size:
            return

        assert n >= 2

        rng = np.arange(n)  # Idx's of remaining indivs

        # Find k nearest others for each individual, mask self in _dist
        k = min(self._params.nb_close, n - 1)
        idx_sorted = (self._dist[:n, :n] + np.eye(n) * 1e9).argsort(-1)
        # idx_closest = idx_sorted[:n, :k]
        # idx_rest = idx_sorted[:, k:]
        # rest_start_idx = np.zeros(n, dtype=int)
        is_removed = np.zeros(n, dtype=bool)
        is_closest = np.zeros((n, n), dtype=bool)
        is_closest[rng[:, None], idx_sorted[:, :k]] = True

        # Make a (cyclic) linked list of the ordered
        prv = np.zeros((n, n), dtype=int)
        nxt = np.zeros((n, n), dtype=int)

        idx_sorted_nxt = np.roll(idx_sorted, -1, axis=-1)
        prv[rng[:, None], idx_sorted_nxt] = idx_sorted
        nxt[rng[:, None], idx_sorted] = idx_sorted_nxt
        # We track a pointer pointing at the indiv which is the least closest
        # of the 'topk' nb_closest (for each individual)
        idx_least_closest = idx_sorted[:, k - 1]
        head = idx_sorted[:, 0]

        dist_closest_sum = self._dist_closest_sum[:n]
        dist_closest_mean = dist_closest_sum / k
        assert np.allclose(
            dist_closest_sum,
            self._dist[rng[:, None], idx_sorted[:, :k]].sum(-1),
        )

        cost_rank = self._cost_rank[:n]

        while len(rng) > self._params.min_pop_size:

            idx_remove = None
            if head is not None:
                idx_duplicate = rng[self._dist[rng, head[rng]] == 0]
                if len(idx_duplicate) > 0:
                    idx_remove = idx_duplicate[0]

                    # Update head of the list where we remove the closest
                    idx_update_head = rng[head[rng] == idx_remove]
                    head[idx_update_head] = nxt[
                        idx_update_head, head[idx_update_head]
                    ]
                else:
                    head = (
                        None  # No longer needed after removing all duplicates
                    )

            if idx_remove is None:
                # TODO can we do more efficient since ordering hardly changes?
                biased_fitness_rng = self._compute_biased_fitness(
                    cost_rank[rng], dist_closest_mean[rng]
                )

                # Find idx of item to remove (with largest biased fitness)
                idx_remove = rng[biased_fitness_rng.argmax()]

            # Update rng with indices of remaining nodes
            is_removed[idx_remove] = True
            (rng,) = np.where(~is_removed)

            if len(rng) <= k:
                # All others were part of closest k (since we had len(rng) + 1
                # indivs so len(rng) others before removing), so we no longer
                # need to track anything
                dist_closest_sum[rng] -= self._dist[rng, idx_remove]
                dist_closest_mean[rng] = dist_closest_sum[rng] / (len(rng) - 1)
            else:

                # Remove from linked list
                prv[rng, nxt[rng, idx_remove]] = prv[rng, idx_remove]
                nxt[rng, prv[rng, idx_remove]] = nxt[rng, idx_remove]

                # If the removed indiv is in the topk, we advance topk pointer
                idx_update_topk = rng[is_closest[rng, idx_remove]]
                new_idx_least_closest = nxt[
                    idx_update_topk, idx_least_closest[idx_update_topk]
                ]
                idx_least_closest[idx_update_topk] = new_idx_least_closest
                is_closest[idx_update_topk, new_idx_least_closest] = True

                # Incrementally compute values for updated topk
                # Compute updated diversity for all (incremental)
                dist_closest_sum[idx_update_topk] += (
                    self._dist[idx_update_topk, new_idx_least_closest]
                    - self._dist[idx_update_topk, idx_remove]
                )
                dist_closest_mean[idx_update_topk] = (
                    dist_closest_sum[idx_update_topk] / k
                )

            # Decrease cost rank of all items ranked after removed indiv
            cost_rank[rng[cost_rank[rng] > cost_rank[idx_remove]]] -= 1

        # Update datastructures
        n = len(rng)
        k = min(self._params.nb_close, n - 1)
        # TODO use np object array?
        self._indivs = [self._indivs[i] for i in rng]

        # Update dist and indexing
        self._dist[:n, :n] = self._dist[np.ix_(rng, rng)]
        self._dist[n:, :] = 0
        self._dist[:, n:] = 0

        self._cost[:n] = self._cost[rng]
        self._cost_rank[:n] = cost_rank[rng]

        rng = np.arange(n)

        # Number of individuals in the subpopulation

        # n x k with indices of closest others
        self._idx_closest[:n, :k] = np.argpartition(
            self._dist[:n, :n] + np.eye(n) * 1e9, k - 1, axis=-1
        )[:, :k]
        # n x k with distances to closest others
        self._dist_closest[:n, :k] = self._dist[
            rng[:, None], self._idx_closest[:n, :k]
        ]
        self._dist_closest_argmax[:n] = self._dist_closest[:n, :k].argmax(-1)
        self._dist_closest_max[:n] = self._dist_closest[
            rng, self._dist_closest_argmax[:n]
        ]

        # Average distance to closest (a.k.a. diversity)
        self._dist_closest_sum[:n] = self._dist_closest[:n, :k].sum(-1)
        dist_closest_mean[:n] = self._dist_closest_sum[:n] / k

        # Update cost rank and compute biased fitness
        k = min(self._params.nb_close, n - 1)  # At most n - 1 others
        diversity = dist_closest_mean[:n]
        self._biased_fitness[:n] = self._compute_biased_fitness(
            self._cost_rank[:n], diversity
        )

        self._check_consistency()


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
        self._best_cost = best.cost()

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
        cost = individual.cost()
        if individual.is_feasible():
            self._feas.add(individual, cost)

            if self._best is None or cost < self._best_cost:
                self._best = individual
                self._best_cost = cost
        else:
            self._infeas.add(individual, cost)

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
                return self._feas[idx]

            return self._infeas[idx - num_feas]

        item1 = select()
        item2 = select()

        if item1.fitness < item2.fitness:
            return item1.individual

        return item2.individual

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
