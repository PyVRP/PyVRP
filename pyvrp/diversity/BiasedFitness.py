from __future__ import annotations

from typing import Iterator, List

import numpy as np


class BiasedFitness:
    def __init__(
        self,
        nb_close,
        max_size,
        nb_elite,
    ):
        """
        Creates a BiasedFitness instance.

        Computes biased fitness based on distances and costs.

        Parameters
        ----------
        nb_close
            Number of closest others to compute diversity.
        max_size
            Maximum number of items that will be added.
        nb_elite
            Number of elite individuals, used for weight in biased fitness.
        """
        self.nb_close = nb_close
        self.max_size = max_size
        self.nb_elite = nb_elite

        n = max_size
        # n x n with distances (differences)
        self._dist = np.zeros((n, n), dtype=float)
        # n x k with indices of closest others
        self._idx_closest = np.zeros((n, self.nb_close), dtype=int)
        # n x k with distances to closest others
        self._dist_closest = np.zeros((n, self.nb_close), dtype=float)
        self._dist_closest_max = np.zeros(n, dtype=float)
        self._dist_closest_argmax = np.zeros(n, dtype=int)
        # Average distance to closest (a.k.a. diversity)
        self._dist_closest_sum = np.zeros(n, dtype=float)

        # Cost/diversity ranks
        self._cost = np.zeros(n, dtype=float)
        self._cost_rank = np.zeros(n, dtype=int)
        self._biased_fitness = np.zeros(n, dtype=float)

        self._size = 0

        self.DO_CHECKS = False

    def __getitem__(self, idx: int) -> float:
        return self._biased_fitness[idx]

    def __iter__(self) -> Iterator[float]:
        return iter(self._biased_fitness[: self._size])

    def __len__(self) -> int:
        return self._size

    def add(
        self,
        cost: float,
        distances: List[float],
    ):
        """
        Adds an item for biased fitness computation.

        Parameters
        ----------
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
            self._cost[n] = cost
            self._size = 1
            # First individual, no diversity computation needed
            return

        # Update the distance matrix
        dist = np.array(distances)
        self._dist[n, :n] = dist
        self._dist[:n, n] = dist

        if n <= self.nb_close:
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
            k = self.nb_close
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
        k = min(self.nb_close, n)  # At most n others with n + 1 pop
        diversity = self._dist_closest_sum[: n + 1] / k
        self._biased_fitness[: n + 1] = self._compute_biased_fitness(
            self._cost_rank[: n + 1], diversity
        )

        # Keep track of size
        self._size += 1

        self._check_consistency()

    def _check_consistency(self):
        if not self.DO_CHECKS:
            return
        n = len(self)
        k = min(self.nb_close, n - 1)
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

        nb_elite = min(self.nb_elite, n)
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
        k = min(self.nb_close, len(self) - 1)
        return self._dist_closest_sum[individual_idx] / k

    def get_biased_fitness(self, individual_idx: int) -> float:
        return self._biased_fitness[individual_idx]

    def purge(self, size: int) -> np.ndarray[int]:
        """
        Performs survivor selection: individuals in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.

        Parameters
        ----------
        size
            Desired size of population after purge.

        Returns
        -------
        np.ndarray[int]
            Array of indices of remaining individuals.
        """

        n = len(self)
        if n <= size:
            return

        assert n >= 2

        rng = np.arange(n)  # Idx's of remaining indivs

        # Find k nearest others for each individual, mask self in _dist
        k = min(self.nb_close, n - 1)
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

        while len(rng) > size:

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

        idx_survivor = rng

        # Update datastructures
        n = len(rng)
        k = min(self.nb_close, n - 1)

        # Update dist and indexing
        self._dist[:n, :n] = self._dist[np.ix_(rng, rng)]
        self._dist[n:, :] = 0
        self._dist[:, n:] = 0

        self._cost[:n] = self._cost[rng]
        self._cost_rank[:n] = cost_rank[rng]

        self._size = n

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
        k = min(self.nb_close, n - 1)  # At most n - 1 others
        diversity = dist_closest_mean[:n]
        self._biased_fitness[:n] = self._compute_biased_fitness(
            self._cost_rank[:n], diversity
        )

        self._check_consistency()

        return idx_survivor
