import argparse
from typing import Optional

from pyvrp import GeneticAlgorithm, Population, read
from pyvrp._lib.hgspy import (
    LocalSearch,
    PenaltyManager,
    XorShift128,
    crossover,
    diversity,
    operators,
)
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
    pen_manager = PenaltyManager(data.vehicle_capacity())

    div_op = diversity.broken_pairs_distance
    pop = Population(data, pen_manager, rng, div_op)
    ls = LocalSearch(data, pen_manager, rng)

    node_ops = [
        operators.Exchange10(data, pen_manager),
        operators.Exchange20(data, pen_manager),
        operators.MoveTwoClientsReversed(data, pen_manager),
        operators.Exchange22(data, pen_manager),
        operators.Exchange21(data, pen_manager),
        operators.Exchange11(data, pen_manager),
        operators.TwoOpt(data, pen_manager),
    ]

    for op in node_ops:
        ls.add_node_operator(op)

    route_ops: list = [
        operators.RelocateStar(data, pen_manager),
        operators.SwapStar(data, pen_manager),
    ]

    for op in route_ops:
        ls.add_route_operator(op)

    crossover_op = crossover.selective_route_exchange
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
