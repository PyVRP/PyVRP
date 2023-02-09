import argparse
from typing import Optional

from pyvrp import (
    GeneticAlgorithm,
    PenaltyManager,
    Population,
    ProblemData,
    XorShift128,
)
from pyvrp.crossover import selective_route_exchange
from pyvrp.diversity import broken_pairs_distance
from pyvrp.educate import NODE_OPERATORS, ROUTE_OPERATORS, LocalSearch
from pyvrp.read import ROUND_FUNCS, read
from pyvrp.stop import MaxIterations, MaxRuntime, StoppingCriterion


def parse_args():
    parser = argparse.ArgumentParser(prog="pyvrp.solve")

    parser.add_argument("data_loc")
    parser.add_argument(
        "--instance_format",
        type=str,
        choices=["vrplib", "solomon"],
        default="vrplib",
    )
    parser.add_argument(
        "--round_func", type=str, default="none", choices=ROUND_FUNCS.keys()
    )

    parser.add_argument("--seed", required=True, type=int)

    stop = parser.add_mutually_exclusive_group(required=True)
    stop.add_argument("--max_runtime", type=float)
    stop.add_argument("--max_iterations", type=int)

    return parser.parse_args()


def solve(
    data: ProblemData,
    seed: int,
    max_runtime: Optional[float] = None,
    max_iterations: Optional[int] = None,
    **kwargs,
):
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
    data = read(args.data_loc, args.instance_format, args.round_func)
    res = solve(data, **vars(args))

    print(res)


if __name__ == "__main__":
    main()
