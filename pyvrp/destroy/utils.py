from pyvrp._pyvrp import (
    ProblemData,
    Route,
    Solution,
)


def remove_clients(
    data: ProblemData, solution: Solution, clients: list[int]
) -> Solution:
    """
    Removes clients from the solution and returns a new solution.
    """
    routes = []
    for route in solution.routes():
        visits = [c for c in route.visits() if c not in clients]

        if visits:  # empty routes are not allowed
            routes.append(Route(data, visits, route.vehicle_type()))

    return Solution(data, routes)
