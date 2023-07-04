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
    sorted_routes = sorted(routes, key=bool)  # move empty routes to the back
    return [client for route in sorted_routes for client in route]
