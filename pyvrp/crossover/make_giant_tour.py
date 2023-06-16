from typing import List

from pyvrp import Route


def make_giant_tour(routes: List[Route]):
    """
    Makes a giant tour of the given routes. The giant tour representation
    is a list of clients, where each client is represented by its index in the
    problem data.
    """
    sorted_routes = sorted(routes, key=bool)
    return [client for route in sorted_routes for client in route]
