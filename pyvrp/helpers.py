from typing import List

from pyvrp import Individual, PenaltyManager, ProblemData

Routes = List[List[int]]


class TooManyRoutesError(Exception):
    """
    Raised when trying to make an Individual using more routes than vehicles.
    """

    pass


def pad_routes(routes: Routes, num_vehicles: int) -> Routes:
    """
    Pads list of routes with empty routes to match a desired number of
    vehicles.

    Parameters
    ----------
    routes
        Routes to be padded with empty routes.
    num_vehicles
        Desired number of routes

    Returns
    -------
    Routes
        Routes object padded with empty routes to match num_vehicles.

    Raises
    ------
    TooManyRoutesError
        Raised when more there are more routes than num_vehicles.
    """
    if len(routes) > num_vehicles:
        raise TooManyRoutesError()
    return routes + [[] for _ in range(len(routes), num_vehicles)]


def make_individual(
    data: ProblemData, pm: PenaltyManager, routes: Routes
) -> Individual:
    """
    Convenience function to create an individual.

    Parameters
    ----------
    data
        ProblemData object for which this individual is a solution.
    pm
        PenaltyManager object for this individual.
    routes
        Routes object as list of list of integers to be converted to a
        solution.

    Returns
    -------
    Individual
        Individual representing a solution based on routes.
    """
    return Individual(data, pm, pad_routes(routes, data.num_vehicles))
