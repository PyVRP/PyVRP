from __future__ import annotations

from dataclasses import dataclass
from typing import List

import numpy as np

from pyvrp._ProblemData import ProblemData

Neighbours = List[List[int]]


@dataclass
class NeighbourhoodParams:
    """
    Configuration for calculating a granular neighbourhood.

    Attributes
    ----------
    weight_wait_time
        TODO
    weight_time_warp
        TODO
    nb_granular
        Number of other clients that are in each client's granular
        neighbourhood. This parameter determines the size of the overall
        neighbourhood.
    symmetric_proximity
        TODO
    symmetric_neighbours
        TODO

    Raises
    ------
    ValueError
        When ``nb_granular`` is non-positive.
    """

    weight_wait_time: float = 0.2
    weight_time_warp: float = 1.0
    nb_granular: int = 40
    symmetric_proximity: bool = True
    symmetric_neighbours: bool = False

    def __post_init__(self):
        if self.nb_granular <= 0:
            raise ValueError("nb_granular <= 0 not understood.")


def compute_neighbours(
    data: ProblemData, *, params: NeighbourhoodParams = NeighbourhoodParams()
) -> Neighbours:
    """
    Computes neighbours defining the neighbourhood for a problem instance.

    Parameters
    ----------
    data
        ProblemData for which to compute the neighbourhood.
    params, optional
        Keyword-only argument for neighbourhood parameters. If not provided,
        a default value will be used.

    Returns
    -------
    Neighbours
        A list of list of integers representing the neighbours for each client.
        The first element represents the depot and is an empty list.
    """

    proximity = _compute_proximity(data, params)

    # Mask self from proximities by adding a large constant
    # and add the index with a very small weight as tie-breaker (to mimic
    # original c++ version)
    n = len(proximity)
    k = min(params.nb_granular, n - 1)
    idcs = np.arange(n)
    jitter = 1e-6 * idcs  # add a bit of jitter to break ties
    proximity = proximity + jitter[None, :]
    np.fill_diagonal(proximity, np.inf)  # exclude self from neighbourhood
    idx_topk = np.argpartition(proximity, k, axis=-1)[:, :k]

    if params.symmetric_neighbours:
        # Convert into adjacency matrix that can be symmetrized
        adj = np.zeros_like(proximity, dtype=bool)
        adj[idcs[:, None], idx_topk] = True

        # Add correlated vertex if correlated in one of two directions
        adj = adj | adj.transpose()

        # Append empty neighbours for depot and add 1 to offset depot indexing
        return [[]] + [(np.flatnonzero(row) + 1).tolist() for row in adj]
    else:
        return [[]] + (np.sort(idx_topk, -1) + 1).tolist()


def _compute_proximity(
    data: ProblemData, params: NeighbourhoodParams = NeighbourhoodParams()
) -> np.ndarray[float]:
    """
    Computes proximity for neighborhood.

    Parameters
    ----------
    data
        ProblemData for which to compute proximity.
    params
        NeighbourhoodParams that define how proximity is computed.

    Returns
    -------
    np.ndarray[float]
        A numpy array of size n x n where n = data.num_clients containing
        the proximities values between all clients (depot excluded).
    """

    dim = data.num_clients + 1

    clients = [data.client(idx) for idx in range(dim)]

    earliest = np.array([cli.tw_early for cli in clients])
    latest = np.array([cli.tw_late for cli in clients])
    service = np.array([cli.service_duration for cli in clients])
    durations = np.array(
        [[data.dist(i, j) for j in range(dim)] for i in range(dim)],
        dtype=float,
    )

    min_wait_time = np.maximum(
        earliest[None, :] - durations - service[:, None] - latest[:, None], 0
    )
    min_time_warp = np.maximum(
        earliest[:, None] + service[:, None] + durations - latest[None, :],
        0,
    )

    prox = (
        durations
        + params.weight_wait_time * min_wait_time
        + params.weight_time_warp * min_time_warp
    )

    if params.symmetric_proximity:
        prox = np.minimum(prox, prox.T)
    return prox[1:, 1:]  # Skip depot
