from dataclasses import dataclass
from typing import List

from pyvrp import ProblemData


@dataclass
class Route:
    clients: List[int]
    time: int
    load: int


def split_giant_tour(tour: List[int], data: ProblemData) -> List[List[int]]:
    """
    Splits a giant tour into a list of routes in a naive way. The giant tour
    sequentially visits all clients until it is no longer feasible to add a
    client to the current route. Then, a new route is started. This is repeated
    until all clients have been visited.
    """
    depot = 0
    prev_client = depot
    route = Route([], data.client(depot).tw_early, 0)
    routes = [route]

    for idx, client in enumerate(tour):
        if len(routes) == data.num_vehicles:
            # If there is only one vehicle left, we have to add all remaining
            # clients to the last route.
            route.clients.extend(tour[idx:])
            break

        client_arrive = route.time + data.duration(prev_client, client)
        client_start = max(client_arrive, data.client(client).tw_early)
        client_finish = client_start + data.client(client).service_duration
        depot_arrive = client_finish + data.duration(client, depot)

        if (
            route.load + data.client(client).demand > data.vehicle_capacity
            or client_arrive > data.client(client).tw_late
            or depot_arrive > data.client(depot).tw_late
        ):
            # Adding the client is not feasible, so we start a new route.
            prev_client = depot
            early = data.client(depot).tw_early + data.duration(depot, client)
            route_start = max(data.client(client).tw_early, early)
            route_load = data.client(client).demand

            route = Route([client], route_start, route_load)
            routes.append(route)
        else:
            prev_client = client
            route.clients.append(client)
            route.time = client_finish
            route.load += data.client(client).demand

    return [route.clients for route in routes if route.clients]
