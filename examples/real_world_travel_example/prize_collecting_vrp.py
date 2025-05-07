import logging
from typing import Any
from pyvrp.stop import MaxRuntime, MaxIterations, MultipleCriteria
from pyvrp import Client, Depot, Model
import itertools
from pyvrp.plotting import plot_solution
import matplotlib.pyplot as plt

from .data import cities, edges


logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

m = Model()

depot = m.add_depot(x=0, y=0, name=cities[0]["name"])

# Vehicle: 1 traveler, start+end at Berlin, max 7 days = 10080 min.
m.add_vehicle_type(
    num_available=1,
    start_depot=depot,
    end_depot=depot,
    max_duration=7 * 24 * 60,  # 7 days.
)

# Add optional clients with prize = popularity and stay time = 1 day.
for cid, info in cities.items():
    if cid == 0:
        continue
    m.add_client(
        x=info["x"],
        y=info["y"],
        prize=info["popularity"],
        service_duration=24 * 60,
        required=False,
        name=info["name"],
    )


def add_edge(frm: Client | Depot, to: Client | Depot, details: dict[str, Any]) -> None:
    # Fast train: shorter duration, higher distance (we ignore price in routing).
    m.add_edge(
        frm,
        to,
        distance=details["fast"][
            "price"
        ],  # Assign the price to the distance because we must minimize the price.
        duration=details["fast"]["duration"],
        profile=details["fast"]["profile"]
    )
    # Slow train: longer duration, same distance.
    m.add_edge(
        frm,
        to,
        # Assign the price to the distance because we must minimize the price.
        distance=details["slow"]["price"],
        duration=details["slow"]["duration"],
        profile=details["slow"]["profile"],
    )


# Add edges for both train modes:
#   - base profile (None) = “fast” trains
#   - slow_profile = “slow” trains
for (i, j), modes in edges.items():
    add_edge(m.locations[i], m.locations[j], modes)
    # To simplify the task, we assume that a return route has the same price/duration.
    add_edge(m.locations[j], m.locations[i], modes)


solution = m.solve(stop=MultipleCriteria([MaxRuntime(2), MaxIterations(500)]))


logger.info(f"Visits: {solution.best.routes()[0].visits()}")
logger.info("Route and chosen trains:")
for route in solution.best.routes():
    pairs = itertools.pairwise(route.visits())
    for arc in pairs:
        logger.info(
            f"{cities[arc[0]]['name']} -> {cities[arc[1]]['name']} via {route.vehicle_type()} train."
        )
    logger.info("End at depot\n")


_, ax = plt.subplots(figsize=(8, 8))
plot_solution(solution.best, m.data(), ax=ax)
plt.show()
