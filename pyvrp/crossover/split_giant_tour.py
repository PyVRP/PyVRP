from dataclasses import dataclass
from typing import List

from pyvrp._ProblemData import ProblemData


@dataclass
class Route:
    clients: List[int]
    time: int
    load: int


def split_giant_tour(
    tour: List[int], vehicle_types: List[int], data: ProblemData
) -> List[List[int]]:
    """
    Splits a giant tour into a list of routes. We start with an empty route and
    sequentially add clients until it is no longer feasible for the given
    route type. We repeat this until all clients are visited, or until all
    vehicle are used.

    Parameters
    ----------
    tour
        A giant tour of client visits.
    vehicle_types
        The vehicle types to use for the routes.
    data
        The problem data.
    """
    routes = []
    depot = 0
    tour_idx = 0

    for idx, vehicle_type_idx in enumerate(vehicle_types):
        vehicle_type = data.vehicle_type(vehicle_type_idx)

        route = Route([], data.client(depot).tw_early, 0)
        prev = depot

        if idx == len(vehicle_types) - 1:
            # If there is only one vehicle left, we add all remaining clients
            # to the last route.
            route.clients.extend(tour[idx:])
            break

        while tour_idx < len(tour):
            client = tour[tour_idx]

            arrive_at_client = route.time + data.duration(prev, client)
            start_at_client = max(
                arrive_at_client, data.client(client).tw_early
            )
            finish_at_client = (
                start_at_client + data.client(client).service_duration
            )
            return_to_depot = finish_at_client + data.duration(client, depot)

            if (
                route.load + data.client(client).demand > vehicle_type.capacity
                or arrive_at_client > data.client(client).tw_late
                or return_to_depot > data.client(depot).tw_late
            ):
                # Adding the current client to the route makes it infeasible,
                # so we stop here and use the next vehicle type.
                routes.append(route)
                break
            else:
                prev = client
                route.clients.append(client)
                route.time = finish_at_client
                route.load += data.client(client).demand

                tour_idx += 1

    return [route.clients for route in routes if route.clients]
