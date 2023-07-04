from typing import List, Tuple

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._ProblemData import ProblemData
from pyvrp._Solution import Solution
from pyvrp._XorShift128 import XorShift128

from .make_giant_tour import make_giant_tour
from .split_giant_tour import split_giant_tour


def ordered_exchange(
    parents: Tuple[Solution, Solution],
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
    tour1 = make_giant_tour(parents[0].get_routes())
    tour2 = make_giant_tour(parents[1].get_routes())

    start, end = _get_start_end(data.num_clients, rng)
    tour = _ordered_exchange(tour1, tour2, data, (start, end))

    return Solution(data, split_giant_tour(tour, data))  # type: ignore


def _get_start_end(num: int, rng) -> Tuple[int, int]:
    start = rng.randint(num)
    end = rng.randint(num)

    while end == start:
        end = rng.randint(num)

    return start, end


def _ordered_exchange(
    tour1: List[int],
    tour2: List[int],
    data: ProblemData,
    offsets: Tuple[int, int],
) -> List[int]:
    """
    Performs the ordered exchange crossover on the given tours and offsets.

    The offsets are the start and end indices (inclusive) of the first
    parent's tour that will be copied into the offspring. The rest of the
    offspring will be filled with the second parent's tour.
    """
    tour = [0] * data.num_clients
    copied = [False] * (data.num_clients + 1)

    start, end = offsets

    def idx2client(idx):
        return idx % data.num_clients

    pos = start
    while idx2client(pos) != idx2client(end + 1):
        tour[idx2client(pos)] = tour1[idx2client(pos)]
        copied[tour[idx2client(pos)]] = True
        pos += 1

    for idx in range(1, data.num_clients + 1):
        client = tour2[idx2client(end + idx)]

        if not copied[client]:
            tour[idx2client(pos)] = client
            pos += 1

    return tour
