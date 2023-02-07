import argparse
from typing import Optional

from pyvrp import (
    GeneticAlgorithm,
    PenaltyManager,
    Population,
    XorShift128,
    read,
)
from pyvrp.crossover import selective_route_exchange
from pyvrp.diversity import broken_pairs_distance
from pyvrp.educate import NODE_OPERATORS, ROUTE_OPERATORS, LocalSearch
from pyvrp.stop import MaxIterations, MaxRuntime, StoppingCriterion


def parse_args():
    parser = argparse.ArgumentParser(prog="pyvrp.solve")

    parser.add_argument("data_loc")
    parser.add_argument("--seed", required=True, type=int)

    stop = parser.add_mutually_exclusive_group(required=True)
    stop.add_argument("--max_runtime", type=float)
    stop.add_argument("--max_iterations", type=int)

    return parser.parse_args()


def solve(
    data_loc: str,
    seed: int,
    max_runtime: Optional[float],
    max_iterations: Optional[int],
    **kwargs,
):
    data = read(data_loc)
    rng = XorShift128(seed=seed)
    pen_manager = PenaltyManager(data.vehicle_capacity)
    pop = Population(data, pen_manager, rng, broken_pairs_distance)
    ls = LocalSearch(data, pen_manager, rng)

    node_ops = [node_op(data, pen_manager) for node_op in NODE_OPERATORS]

    for op in node_ops:
        ls.add_node_operator(op)

    route_ops = [route_op(data, pen_manager) for route_op in ROUTE_OPERATORS]

    for op in route_ops:
        ls.add_route_operator(op)

    crossover_op = selective_route_exchange
    algo = GeneticAlgorithm(data, pen_manager, rng, pop, ls, crossover_op)

    if max_runtime is not None:
        stop: StoppingCriterion = MaxRuntime(max_runtime)
    else:
        assert max_iterations is not None
        stop = MaxIterations(max_iterations)

    return algo.run(stop)


def main():
    args = parse_args()
    res = solve(**vars(args))

    print(res)


if __name__ == "__main__":
    main()
