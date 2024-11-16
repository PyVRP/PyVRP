from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
)


def concentric(
    data: ProblemData,
    solution: Solution,
    cost_eval: CostEvaluator,
    rng: RandomNumberGenerator,
    neighbours: list[list[int]],
):
    """
    Removes a number of clients that are closest to a randomly selected client.

    Parameters
    ----------
    data
        The problem data.
    solution
        The solution to destroy.
    cost_eval
        The cost evaluator.
    rng
        The random number generator.
    neighbours
        The neighbourhood to use.
    """
    num_destroy = rng.randint(15) + 10

    # Find all client indices to remove.
    client = rng.randint(data.num_clients) + 1
    closest = data.distance_matrix(0)[client].argsort().tolist()
    closest.remove(client)
    top_k = closest[:num_destroy]

    # Rebuild the Solution but remove selected clients.
    routes = []
    for route in solution.routes():
        if visits := [idx for idx in route.visits() if idx not in top_k]:
            routes.append(Route(data, visits, route.vehicle_type()))

    return Solution(data, routes)
