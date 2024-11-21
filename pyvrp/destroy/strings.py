from dataclasses import dataclass

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
)
from pyvrp.destroy.utils import remove_clients


@dataclass
class SISRParams:
    """
    Parameters for the SISR operator.

    Parameters
    ----------
    max_string_size
        Maximum size of the string to remove.
    split_probability
        Probability of selecting the split-string operator.
    """

    max_string_size: int = 10
    split_probability: float = 0.5
    count_probability: float = 0.1


class SISR:
    """
    Slack Inducing String Removal heuristic.

    Parameters
    ----------
    params
        Parameters for the SISR operator.
    """

    def __init__(self, params: SISRParams = SISRParams()):
        self._params = params

    def __call__(
        self,
        data: ProblemData,
        solution: Solution,
        cost_eval: CostEvaluator,
        rng: RandomNumberGenerator,
        neighbors: list[list[int]],
        num_destroy: int,
    ):
        """
        Destroys a solution by removing a number of strings.
        """
        avg_route_size = abs(solution.num_clients() // solution.num_routes())
        max_string_size = min(self._params.max_string_size, avg_route_size)
        max_num_strings = round((4 * num_destroy) / (max_string_size + 1) - 1)
        num_strings = rng.randint(max_num_strings) + 1

        # Select a random client to start the removal process.
        routes = solution.routes()
        clients = [c for route in routes for c in route.visits()]
        center = clients[rng.randint(len(clients))]

        removed = {center}  # clients of removed strings
        ignored = {center}  # clients of destroyed routes

        for _ in range(num_strings):
            for neighbor in neighbors[center]:
                if neighbor not in ignored:
                    route = next(r for r in routes if neighbor in r.visits())
                    size = rng.randint(min(len(route), max_string_size)) + 1
                    string = self._select_string(route, neighbor, size, rng)

                    removed.update(string)
                    ignored.update(route.visits())
                    break

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
            self.sequential_string(route, client, size, rng)
            if rng.rand() < self._params.split_probability
            else self._split_string(route, client, size, rng)
        )

    def sequential_string(
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

        string = self.sequential_string(route, client, size + m, rng)
        split_at = rng.randint(len(string) - m + 1)
        return string[:split_at] + string[split_at + m :]
