from __future__ import annotations

from typing import TYPE_CHECKING

from pyvrp.search._search import NeighbourhoodParams
from pyvrp.search._search import compute_neighbours as _compute_neighbours

if TYPE_CHECKING:
    from pyvrp import Activity, ProblemData


def compute_neighbours(
    data: ProblemData, params: NeighbourhoodParams = NeighbourhoodParams()
) -> dict[Activity, list[Activity]]:
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
    dict
        A mapping from activities to neighbouring activities for each client
        and pickup.
    """
    return _compute_neighbours(data, params)  # delegate to C++ implementation
