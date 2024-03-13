from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

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
) -> list[list[int]]:
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
        The first lists in the lower indices are associated with the depots and
        are all empty.
    """
    proximity = _compute_proximity(
        data,
        params.weight_wait_time,
        params.weight_time_warp,
    )

    if params.symmetric_proximity:
        proximity = np.minimum(proximity, proximity.T)

    for group in data.groups():
        if group.mutually_exclusive:
            # Clients in mutually exclusive groups cannot neighbour each other,
            # since only one of them can be in the solution at any given time.
            # We use max float, not infty, to ensure these clients are ordered
            # before the depots: we want to avoid same group neighbours, but it
            # is not problematic if we need to have them.
            idcs = np.ix_(group.clients, group.clients)
            proximity[idcs] = np.finfo(np.float64).max

    np.fill_diagonal(proximity, np.inf)  # cannot be in own neighbourhood
    proximity[: data.num_depots, :] = np.inf  # depots have no neighbours
    proximity[:, : data.num_depots] = np.inf  # clients do not neighbour depots

    k = min(params.nb_granular, data.num_clients - 1)  # excl. self
    top_k = np.argsort(proximity, axis=1, kind="stable")[data.num_depots :, :k]

    if not params.symmetric_neighbours:
        return [[] for _ in range(data.num_depots)] + top_k.tolist()

    # Construct a symmetric adjacency matrix and return the adjacent clients
    # as the neighbourhood structure.
    adj = np.zeros_like(proximity, dtype=bool)
    rows = np.expand_dims(np.arange(data.num_depots, len(proximity)), axis=1)
    adj[rows, top_k] = True
    adj = adj | adj.transpose()

    return [np.flatnonzero(row).tolist() for row in adj]


def _compute_proximity(
    data: ProblemData, weight_wait_time: float, weight_time_warp: float
) -> np.ndarray[float]:
    """
    Computes proximity for neighborhood. Proximity is based on [1]_, with
    modification for additional VRP variants.

    Parameters
    ----------
    data
        ProblemData for which to compute proximity.
    params
        NeighbourhoodParams that define how proximity is computed.

    Returns
    -------
    np.ndarray[float]
        An array of size :py:attr:`~pyvrp._pyvrp.ProblemData.num_locations`
        by :py:attr:`~pyvrp._pyvrp.ProblemData.num_locations`.

    References
    ----------
    .. [1] Vidal, T., Crainic, T. G., Gendreau, M., and Prins, C. (2013). A
           hybrid genetic algorithm with adaptive diversity management for a
           large class of vehicle routing problems with time-windows.
           *Computers & Operations Research*, 40(1), 475 - 489.
    """
    clients = [data.location(loc) for loc in range(data.num_locations)]

    early = np.asarray([c.tw_early for c in clients])
    late = np.asarray([c.tw_late for c in clients])

    service = np.zeros_like(early)
    service[data.num_depots :] = [c.service_duration for c in data.clients()]

    prize = np.zeros_like(early)
    prize[data.num_depots :] = [client.prize for client in data.clients()]

    duration = data.duration_matrix()

    # Minimum wait time and time warp of visiting j directly after i.
    min_wait = early[None, :] - duration - service[:, None] - late[:, None]
    min_tw = early[:, None] + service[:, None] + duration - late[None, :]

    return (
        np.asarray(data.distance_matrix(), dtype=float)
        + weight_wait_time * np.maximum(min_wait, 0)
        + weight_time_warp * np.maximum(min_tw, 0)
        - prize[None, :]
    )
