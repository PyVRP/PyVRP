#!/usr/bin/env python

import argparse
import pathlib
from functools import partial

import numpy as np
from tqdm.contrib.concurrent import process_map

from pyvrp.solve import solve


def tabulate(headers, rows) -> str:
    # These lengths are used to space each column properly.
    lengths = [len(header) for header in headers]

    for row in rows:
        for idx, cell in enumerate(row):
            lengths[idx] = max(lengths[idx], len(str(cell)))

    header = [
        "  ".join(f"{hdr:<{ln}s}" for ln, hdr in zip(lengths, headers)),
        "  ".join("-" * ln for ln in lengths),
    ]

    content = [
        "  ".join(f"{str(c):>{ln}s}" for ln, c in zip(lengths, row))
        for row in rows
    ]

    return "\n".join(header + content)


def parse_args():
    parser = argparse.ArgumentParser(prog="pyvrp.benchmark")

    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--instances", required=True, nargs="+")
    parser.add_argument("--num_procs", type=int, default=4)

    stop = parser.add_mutually_exclusive_group(required=True)
    stop.add_argument("--max_runtime", type=float)
    stop.add_argument("--max_iterations", type=int)

    return parser.parse_args()


def _solve(data_loc, *args, **kwargs):
    res = solve(data_loc, *args, **kwargs)
    instance_name = pathlib.Path(data_loc).stem

    return (
        instance_name,
        "Y" if res.is_feasible() else "N",
        int(res.cost()),
        res.num_iterations,
        round(res.runtime, 3),
    )


def main():
    args = parse_args()

    func = partial(_solve, **vars(args))
    func_args = sorted(args.instances)

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
