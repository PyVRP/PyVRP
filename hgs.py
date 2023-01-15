#!/usr/bin/env python

import argparse
from time import perf_counter

from test.tools import get_hgspy


def parse_args():
    parser = argparse.ArgumentParser(prog="hgs")

    parser.add_argument("data_loc")
    parser.add_argument("res_loc")
    parser.add_argument("seed", type=int)
    parser.add_argument("--debug", action="store_true")

    # TODO config

    stop = parser.add_mutually_exclusive_group(required=True)
    stop.add_argument("--max_runtime", type=float)
    stop.add_argument("--max_iterations", type=int)

    return parser.parse_args()


def main():
    args = parse_args()

    if args.debug:
        hgspy = get_hgspy("debug/lib/hgspy*.so")
    else:
        hgspy = get_hgspy("release/lib/hgspy*.so")

    data = hgspy.ProblemData.from_file(args.data_loc)
    rng = hgspy.XorShift128(seed=args.seed)
    pen_manager = hgspy.PenaltyManager(data.vehicle_capacity())
    diversity = hgspy.diversity.broken_pairs_distance
    pop = hgspy.Population(data, pen_manager, rng, diversity)
    ls = hgspy.LocalSearch(data, pen_manager, rng)

    node_ops = [
        hgspy.operators.Exchange10(data, pen_manager),
        hgspy.operators.Exchange20(data, pen_manager),
        hgspy.operators.MoveTwoClientsReversed(data, pen_manager),
        hgspy.operators.Exchange22(data, pen_manager),
        hgspy.operators.Exchange21(data, pen_manager),
        hgspy.operators.Exchange11(data, pen_manager),
        hgspy.operators.TwoOpt(data, pen_manager),
    ]

    for op in node_ops:
        ls.add_node_operator(op)

    route_ops = [
        hgspy.operators.RelocateStar(data, pen_manager),
        hgspy.operators.SwapStar(data, pen_manager),
    ]

    for op in route_ops:
        ls.add_route_operator(op)

    crossover = hgspy.crossover.selective_route_exchange
    algo = hgspy.GeneticAlgorithm(data, pen_manager, rng, pop, ls, crossover)

    if args.max_runtime is not None:
        stop = hgspy.stop.MaxRuntime(args.max_runtime)
    else:
        stop = hgspy.stop.MaxIterations(args.max_iterations)

    start = perf_counter()
    res = algo.run(stop)
    run_time = round(perf_counter() - start, 2)

    best = res.get_best_found()
    best.to_file(args.res_loc, run_time)


if __name__ == "__main__":
    main()
