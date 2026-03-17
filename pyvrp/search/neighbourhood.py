from __future__ import annotations

from typing import TYPE_CHECKING

from pyvrp.search._search import NeighbourhoodParams
from pyvrp.search._search import compute_neighbours as _compute_neighbours

if TYPE_CHECKING:
    from pyvrp import ProblemData


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
    """
    return _compute_neighbours(data, params)  # delegate to C++ implementation
