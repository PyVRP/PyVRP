import argparse
import pathlib
from functools import partial
from typing import List, Optional

import numpy as np

from pyvrp.educate.neighbourhood import compute_neighbours

try:
    from tqdm.contrib.concurrent import process_map
except ModuleNotFoundError:
    raise ModuleNotFoundError("Install 'tqdm' to use the commandline program.")

from pyvrp import (
    GeneticAlgorithm,
    PenaltyManager,
    Population,
    Result,
    XorShift128,
)
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.educate import NODE_OPERATORS, ROUTE_OPERATORS, LocalSearch
from pyvrp.read import INSTANCE_FORMATS, ROUND_FUNCS, read
from pyvrp.stop import MaxIterations, MaxRuntime


def tabulate(headers: List[str], rows: np.ndarray) -> str:
    """
    Creates a simple table from the given header and row data.
    """
    # These lengths are used to space each column properly.
    lens = [len(header) for header in headers]

    for row in rows:
        for idx, cell in enumerate(row):
            lens[idx] = max(lens[idx], len(str(cell)))

    header = [
        "  ".join(f"{hdr:<{ln}s}" for ln, hdr in zip(lens, headers)),
        "  ".join("-" * ln for ln in lens),
    ]

    content = [
        "  ".join(f"{str(c):>{ln}s}" for ln, c in zip(lens, r)) for r in rows
    ]

    return "\n".join(header + content)


def solve(
    data_loc: str,
    instance_format: str,
    round_func: str,
    seed: int,
    max_runtime: Optional[float],
    max_iterations: Optional[int],
    **kwargs,
) -> Result:
    """
    Solves a single VRPLIB instance.

    Parameters
    ----------
    data_loc
        Filesystem location of the VRPLIB instance.
    instance_format
        Data format of the filesystem instance. Argument is passed to
        ``read()``.
    round_func
        Rounding function to use for rounding non-integral data. Argument is
        passed to ``read()``.
    seed
        Seed to use for the RNG.
    max_runtime
        Maximum runtime (in seconds) for solving. Either ``max_runtime`` or
        ``max_iterations`` must be specified.
    max_iterations
        Maximum number of iterations for solving. Either ``max_runtime`` or
        ``max_iterations`` must be specified.

    Returns
    -------
    Result
        Object storing the solver outcome.
    """
    data = read(data_loc, instance_format, round_func)
    rng = XorShift128(seed=seed)
    pen_manager = PenaltyManager(data.vehicle_capacity)
    pop = Population(data, pen_manager, rng, bpd)
    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, pen_manager, neighbours, rng)

    node_ops = [node_op(data, pen_manager) for node_op in NODE_OPERATORS]

    for op in node_ops:
        ls.add_node_operator(op)

    route_ops = [route_op(data, pen_manager) for route_op in ROUTE_OPERATORS]

    for op in route_ops:
        ls.add_route_operator(op)

    algo = GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex)

    if max_runtime is not None:
        stop = MaxRuntime(max_runtime)
    else:
        assert max_iterations is not None
        stop = MaxIterations(max_iterations)  # type: ignore

    return algo.run(stop)


def benchmark_solve(instance: str, **kwargs):
    """
    Small wrapper script around ``solve()`` that translates result objects into
    a few key statistics, and returns those.
    """
    res = solve(instance, **kwargs)
    instance_name = pathlib.Path(instance).stem

    return (
        instance_name,
        "Y" if res.is_feasible() else "N",
        int(res.cost()),
        res.num_iterations,
        round(res.runtime, 3),
    )


def benchmark(instances: List[str], **kwargs):
    """
    Solves a list of instances, and prints a table with the results. Any
    additional keyword arguments are passed to ``solve()``.

    Parameters
    ----------
    instances
        Paths to the VRPLIB instances to solve.
    """
    func = partial(benchmark_solve, **kwargs)
    func_args = sorted(instances)

    tqdm_kwargs = dict(max_workers=kwargs.get("num_procs", 1), unit="instance")
    data = process_map(func, func_args, **tqdm_kwargs)

    dtypes = [
        ("inst", "U37"),
        ("ok", "U1"),
        ("obj", int),
        ("iters", int),
        ("time", float),
    ]

    data = np.asarray(data, dtype=dtypes)
    headers = ["Instance", "OK", "Obj.", "Iters. (#)", "Time (s)"]

    print("\n", tabulate(headers, data), "\n", sep="")
    print(f"      Avg. objective: {data['obj'].mean():.0f}")
    print(f"     Avg. iterations: {data['iters'].mean():.0f}")
    print(f"   Avg. run-time (s): {data['time'].mean():.2f}")
    print(f"        Total not OK: {np.count_nonzero(data['ok'] == 'N')}")


def run(instances: List[str], **kwargs):
    if len(instances) > 1:
        benchmark(instances, **kwargs)
    else:
        res = solve(instances[0], **kwargs)
        print(res)


def main():
    description = """
    This program is a command line interface for solving CVRP and VRPTW
    instances, specified in VRPLIB format. The program can solve one or
    multiple such instances, and outputs useful information in either
    case.
    """
    parser = argparse.ArgumentParser(prog="pyvrp", description=description)

    parser.add_argument("instances", nargs="+", help="Instance paths.")

    parser.add_argument(
        "--instance_format",
        default="vrplib",
        choices=INSTANCE_FORMATS,
        help="File format. Default 'vrplib'.",
    )

    parser.add_argument(
        "--round_func",
        default="none",
        choices=ROUND_FUNCS.keys(),
        help="Round function to apply for non-integral data. Default 'none'.",
    )

    msg = "Seed to use for reproducible results."
    parser.add_argument("--seed", required=True, type=int, help=msg)

    msg = "Number of processors to use for solving instances. Default 1."
    parser.add_argument("--num_procs", type=int, default=1, help=msg)

    stop = parser.add_mutually_exclusive_group(required=True)

    msg = "Maximum runtime for each instance, in seconds."
    stop.add_argument("--max_runtime", type=float, help=msg)

    msg = "Maximum number of iterations for solving each instance."
    stop.add_argument("--max_iterations", type=int, help=msg)

    run(**vars(parser.parse_args()))


if __name__ == "__main__":
    main()
