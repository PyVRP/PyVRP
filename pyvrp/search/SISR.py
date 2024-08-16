from dataclasses import dataclass

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
)
from pyvrp.destroy.utils import remove_clients
from pyvrp.repair import greedy_repair


@dataclass
class SISRParams:
    """
    Parameters for the SISR operator.

    Parameters
    ----------
    max_string_size
        Maximum size of the string to remove.
    avg_removals
        Average number clients to remove in total.
    split_probability
        Probability of selecting the split-string operator.
    """

    max_string_size: int = 10
    avg_removals: int = 10
    split_probability: float = 0.5
    count_probability: float = 0.1


class SISR:
    """
    Slack Inducing String Removal heuristic.

    Parameters
    ----------
    rng
        Random number generator.
    neighbors
        A list of neighbors for each client.
    params
        Parameters for the operator.
    """

    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        neighbors: list[list[int]],
        params: SISRParams = SISRParams(),
    ):
        self._data = data
        self._rng = rng
        self._neighbors = neighbors
        self._params = params

    def __call__(self, solution: Solution, cost_eval: CostEvaluator):
        destroyed = self.destroy(self._data, solution, cost_eval, self._rng)
        return self.repair(self._data, destroyed, cost_eval, self._rng)

    def repair(
        self,
        data: ProblemData,
        solution: Solution,
        cost_eval: CostEvaluator,
        rng: RandomNumberGenerator,
    ):
        """
        Small wrapper around ``greedy_repair`` to have Solution's as input and
        output.
        """
        visited = {client for r in solution.routes() for client in r.visits()}
        clients = range(data.num_depots, data.num_locations)
        # TODO shuffle?
        unplanned = [idx for idx in clients if idx not in visited]

        # Pad the solution with empty routes to match the number of vehicles.
        num_routes_left = data.num_vehicles - len(solution.routes())
        empty = [Route(data, [], 0) for _ in range(num_routes_left)]

        routes = greedy_repair(
            solution.routes() + empty,
            unplanned,
            data,
            cost_eval,
            self._neighbors,
        )

        return Solution(data, [route for route in routes if route.visits()])

    def destroy(
        self,
        data: ProblemData,
        solution: Solution,
        cost_eval: CostEvaluator,
        rng: RandomNumberGenerator,
    ):
        """
        Destroys a solution by removing a number of strings.
        """
        avg_route_size = abs(solution.num_clients() // solution.num_routes())
        max_string_size = min(self._params.max_string_size, avg_route_size)
        max_num_strings = round(
            (4 * self._params.avg_removals) / (max_string_size + 1) - 1
        )
        num_strings = rng.randint(max_num_strings) + 1

        # Select a random client to start the removal process.
        routes = solution.routes()
        clients = [c for route in routes for c in route.visits()]
        client = clients[rng.randint(len(clients))]

        removed = {client}  # remove clients after strings are selected
        ignored = {client}  # ignore clients of destroyed routes

        for _ in range(num_strings):
            for neighbor in self._neighbors[client]:
                if neighbor not in ignored:
                    route = next(r for r in routes if neighbor in r.visits())
                    size = rng.randint(min(len(route), max_string_size)) + 1
                    string = self._select_string(route, neighbor, size, rng)

                    removed.update(string)
                    ignored.update(route.visits())
                    client = neighbor
                    break

            # TODO what if no eligible neighbor is found?
            # NOTE this cannot happen if the neighbourhood is completee
            # bcs that means there are no more clients to remove

        # print(f"Removed {len(removed)} clients.")
        return remove_clients(data, solution, list(removed))

    def _select_string(
        self, route: Route, client: int, size: int, rng: RandomNumberGenerator
    ) -> list[int]:
        """
        Selects a string of clients from the route to remove.
        """
        if len(route) == 1:
            return [client]

        return (
            self._string(route, client, size, rng)
            if rng.rand() < self._params.split_probability
            else self._split_string(route, client, size, rng)
        )

    def _string(
        self, route: Route, client: int, size: int, rng: RandomNumberGenerator
    ):
        """
        Selects a string of given size from the route that contains the client.
        """
        if size >= len(route):
            return route.visits()

        visits = route.visits()
        pos = rng.randint(size - 1)  # the client position in the string
        start = visits.index(client) - pos

        return [visits[(start + idx) % len(route)] for idx in range(size)]

    def _split_string(
        self, route: Route, client: int, size: int, rng: RandomNumberGenerator
    ):
        """
        Split the string that contains the client into two strings.
        """
        # Determine the length of the substring that splits the string.
        m = 1
        while rng.rand() > self._params.count_probability:
            m += 1
            if m + size == len(route):
                break

        string = self._string(route, client, size + m, rng)
        split_at = rng.randint(len(string) - m + 1)
        return string[:split_at] + string[split_at + m :]
