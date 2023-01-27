#!/usr/bin/env python

import argparse
import glob
import importlib.machinery
import importlib.util
import pathlib
import re
from functools import partial
from time import perf_counter
from typing import Optional

import numpy as np
from tqdm.contrib.concurrent import process_map


def get_hgspy(where: str):
    lib_path = next(glob.iglob(where))
    loader = importlib.machinery.ExtensionFileLoader("hgspy", lib_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    hgspy = importlib.util.module_from_spec(spec)
    loader.exec_module(hgspy)

    return hgspy


def name2size(name: str) -> int:
    """
    Extracts the instance size (i.e., num clients) from the instance name.
    """
    return int(re.search(r'-n(\d{1,3})-', name).group(1))


def static_time_limit(n_clients: int, phase: str) -> int:
    """
    Returns the time limit (in seconds) for solving the static problem for
    the passed-in number of clients and competition phase.

    Instances are grouped into categories of <300/300-500/>500 customers.
    - 3/5/8 minutes for the quali(fication) phase
    - 5/10/15 minutes for the final phase
    """
    if phase not in ["quali", "final"]:
        raise ValueError(f"Invalid phase: {phase}")

    if n_clients < 300:
        return 180 if phase == "quali" else 300
    elif 300 <= n_clients <= 500:
        return 300 if phase == "quali" else 600
    else:
        return 480 if phase == "quali" else 900


def tabulate(headers, rows) -> str:
    # These lengths are used to space each column properly.
    lengths = [len(header) for header in headers]

    for row in rows:
        for idx, cell in enumerate(row):
            lengths[idx] = max(lengths[idx], len(str(cell)))

    header = [
        "  ".join(f"{h:<{l}s}" for l, h in zip(lengths, headers)),
        "  ".join("-" * l for l in lengths),
    ]

    content = ["  ".join(f"{str(c):>{l}s}"
                         for l, c in zip(lengths, row))
               for row in rows]

    return "\n".join(header + content)


def parse_args():
    parser = argparse.ArgumentParser(prog="hgs")

    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--num_procs", type=int, default=4)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument(
        "--instance_pattern", default="instances/ORTEC-VRPTW-ASYM-*.txt"
    )

    stop = parser.add_mutually_exclusive_group(required=True)
    stop.add_argument("--phase", choices=["quali", "final"])
    stop.add_argument("--max_runtime", type=float)
    stop.add_argument("--max_iterations", type=int)

    return parser.parse_args()


def solve(
        data_loc: str,
        seed: int,
        debug: bool,
        phase: Optional[str],
        max_runtime: Optional[float],
        max_iterations: Optional[int],
        **kwargs,
):
    where = pathlib.Path(data_loc)

    if debug:
        hgspy = get_hgspy("debug/lib/hgspy*.so")
    else:
        hgspy = get_hgspy("release/lib/hgspy*.so")

    data = hgspy.ProblemData.from_file(str(where))
    rng = hgspy.XorShift128(seed=seed)
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
        # hgspy.operators.RelocateStar(data, pen_manager),
        # hgspy.operators.SwapStar(data, pen_manager),
    ]

    for op in route_ops:
        ls.add_route_operator(op)

    crossover = hgspy.crossover.selective_route_exchange
    algo = hgspy.GeneticAlgorithm(data, pen_manager, rng, pop, ls, crossover)

    if phase is not None:
        t_lim = static_time_limit(name2size(data_loc), phase)
        stop = hgspy.stop.MaxRuntime(t_lim)
    elif max_runtime is not None:
        stop = hgspy.stop.MaxRuntime(max_runtime)
    else:
        stop = hgspy.stop.MaxIterations(max_iterations)

    start = perf_counter()

    res = algo.run(stop)
    best = res.get_best_found()

    run_time = round(perf_counter() - start, 2)

    return (
        where.stem,
        "Y" if best.is_feasible() else "N",
        int(best.cost()),
        res.get_iterations(),
        round(run_time, 3),
    )


def main():
    args = parse_args()

    func = partial(solve, **vars(args))
    func_args = sorted(glob.glob(args.instance_pattern), key=name2size)

    tqdm_kwargs = dict(max_workers=args.num_procs, unit="instance")
    data = process_map(func, func_args, **tqdm_kwargs)

    dtypes = [
        ("inst", "U37"),
        ("ok", "U1"),
        ("obj", int),
        ("iters", int),
        ("time", float),
    ]

    data = np.asarray(data, dtype=dtypes)

    headers = [
        "Instance",
        "OK",
        "Objective",
        "Iters. (#)",
        "Time (s)",
    ]

    table = tabulate(headers, data)

    print("\n", table, "\n", sep="")

    obj_all = data["obj"]
    obj_feas = data[data["ok"] == "Y"]["obj"]

    print(f"      Avg. objective: {obj_all.mean():.0f}", end=" ")
    print(f"(w/o infeas: {obj_feas.mean():.0f})" if obj_feas.size > 0 else "")

    print(f"     Avg. iterations: {data['iters'].mean():.0f}")
    print(f"   Avg. run-time (s): {data['time'].mean():.2f}")
    print(f"        Total not OK: {np.count_nonzero(data['ok'] == 'N')}")


if __name__ == "__main__":
    main()
