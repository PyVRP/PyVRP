from typing import List, Tuple

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._Individual import Individual
from pyvrp._ProblemData import ProblemData
from pyvrp._XorShift128 import XorShift128

from .make_giant_tour import make_giant_tour
from .split_giant_tour import split_giant_tour


def ordered_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    rng: XorShift128,
):
    """
    Performs an ordered crossover of the given parents. Each parent's routes
    are converted into a giant tour. The crossover randomly selects a subset
    of consecutive clients from the first parent, and the rest comes from the
    second parent.
    """
    first, second = parents
    tour1, tour2 = make_giant_tour(first), make_giant_tour(second)

    start, end = get_start_end(data.num_clients, rng)
    offspring_tour = doExchange(tour1, tour2, data, (start, end))
    offspring_routes = split_giant_tour(offspring_tour, data, cost_evaluator)

    return Individual(data, offspring_routes)


def get_start_end(num: int, rng) -> Tuple[int, int]:
    start = rng.randint(num)
    end = rng.randint(num)

    while end == start:
        end = rng.randint(num)

    return start, end


def doExchange(
    tour1: List[int],
    tour2: List[int],
    data: ProblemData,
    offsets: Tuple[int, int],
) -> List[int]:
    """
    Performs the ordered exchange crossover on the given tours.
    """
    offspring_tour = [0] * data.num_clients
    copied = [False] * (data.num_clients + 1)

    start, end = offsets

    def idx2client(idx):
        return idx % data.num_clients

    insertPos = start
    while idx2client(insertPos) != idx2client(end + 1):
        offspring_tour[idx2client(insertPos)] = tour1[idx2client(insertPos)]
        copied[offspring_tour[idx2client(insertPos)]] = True
        insertPos += 1

    for idx in range(1, data.num_clients + 1):
        client = tour2[idx2client(end + idx)]

        if not copied[client]:
            offspring_tour[idx2client(insertPos)] = client
            insertPos += 1

    return offspring_tour
