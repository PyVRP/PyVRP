from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, List

import numpy as np

if TYPE_CHECKING:
    from pyvrp import ProblemData


@dataclass
class NeighbourhoodParams:
    """
    Configuration for calculating a granular neighbourhood.

    Attributes
    ----------
    weight_wait_time
        Weight given to the minimum wait time aspect of the proximity
        calculation. A large wait time indicates the clients are far apart
        in duration/time.
    weight_time_warp
        Weight given to the minimum time warp aspect of the proximity
        calculation. A large time warp indicates the clients are far apart in
        duration/time.
    nb_granular
        Number of other clients that are in each client's granular
        neighbourhood. This parameter determines the size of the overall
        neighbourhood.
    symmetric_proximity
        Whether to calculate a symmetric proximity matrix. This ensures edge
        :math:`(i, j)` is given the same weight as :math:`(j, i)`.
    symmetric_neighbours
        Whether to symmetrise the neighbourhood structure. This ensures that
        when edge :math:`(i, j)` is in, then so is :math:`(j, i)`. Note that
        this is *not* the same as ``symmetric_proximity``.

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
    data: ProblemData, params: NeighbourhoodParams = NeighbourhoodParams()
) -> List[List[int]]:
    """
    Computes neighbours defining the neighbourhood for a problem instance.

    Parameters
    ----------
    data
        ProblemData for which to compute the neighbourhood.
    params
        NeighbourhoodParams that define how the neighbourhood is computed.

    Returns
    -------
    list
        A list of list of integers representing the neighbours for each client.
        The first element represents the depot and is an empty list.
    """
    proximity = _compute_proximity(
        data,
        params.weight_wait_time,
        params.weight_time_warp,
    )

    if params.symmetric_proximity:
        proximity = np.minimum(proximity, proximity.T)

    # TODO generalise this when we have multiple depots
    n = len(proximity)
    k = min(params.nb_granular, n - 2)  # excl. depot and self

    np.fill_diagonal(proximity, np.inf)  # cannot be in own neighbourhood
    proximity[0, :] = np.inf  # depot has no neighbours
    proximity[:, 0] = np.inf  # clients do not neighbour depot

    top_k = np.argsort(proximity, axis=1, kind="stable")[1:, :k]  # excl. depot

    if not params.symmetric_neighbours:
        return [[], *top_k.tolist()]

    # Construct a symmetric adjacency matrix and return the adjacent clients
    # as the neighbourhood structure.
    adj = np.zeros_like(proximity, dtype=bool)
    rows = np.expand_dims(np.arange(1, n), axis=1)
    adj[rows, top_k] = True
    adj = adj | adj.transpose()

    return [np.flatnonzero(row).tolist() for row in adj]


def _compute_proximity(
    data: ProblemData, weight_wait_time: float, weight_time_warp: float
) -> np.ndarray[float]:
    """
    Computes proximity for neighborhood. Proximity is based on Vidal et al.
    (2013).

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

    References
    ----------
    .. [1] Vidal, T., Crainic, T. G., Gendreau, M., and Prins, C. (2013). A
           hybrid genetic algorithm with adaptive diversity management for a
           large class of vehicle routing problems with time-windows.
           *Computers & Operations Research*, 40(1), 475 - 489.
    """
    dim = data.num_clients + 1
    clients = [data.client(idx) for idx in range(dim)]

    earliest = np.array([client.tw_early for client in clients])
    latest = np.array([client.tw_late for client in clients])
    service = np.array([client.service_duration for client in clients])
    prizes = np.array([client.prize for client in clients])
    durations = np.array(
        [[data.duration(i, j) for j in range(dim)] for i in range(dim)],
        dtype=float,
    )

    min_wait_time = np.maximum(
        earliest[None, :] - durations - service[:, None] - latest[:, None], 0
    )

    min_time_warp = np.maximum(
        earliest[:, None] + service[:, None] + durations - latest[None, :],
        0,
    )

    distances = np.array(
        [[data.dist(i, j) for j in range(dim)] for i in range(dim)],
        dtype=float,
    )

    return (
        distances
        + weight_wait_time * min_wait_time
        + weight_time_warp * min_time_warp
        - prizes[None, :]
    )
