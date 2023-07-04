from typing import List


def make_giant_tour(routes: List[List[int]]) -> List[int]:
    """
    Makes a giant tour of the passed-in routes.

    Parameters
    ----------
    routes
        The routes to be merged into a giant tour.

    Returns
    -------
    List[int]
        The giant tour representation of the given routes.
    """
    return [client for route in routes for client in route]
