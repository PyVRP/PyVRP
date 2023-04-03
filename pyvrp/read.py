import functools
import pathlib
from typing import Callable, Dict, List, Optional, Union

import numpy as np
import vrplib

from ._ProblemData import ProblemData
from ._precision import PRECISION

_Routes = List[List[int]]
_RoundingFunc = Callable[[np.ndarray], np.ndarray]

_INT_MAX = np.iinfo(np.int32).max


def _safe_convert_to_int(vals: np.ndarray) -> np.ndarray:
    # Convert to integer and check that we don't lose precision
    vals_int = vals.astype(int)
    if not np.allclose(vals, vals_int):
        raise ValueError("Data is not integral, please use rounding.")
    return vals_int


def trunc(vals: np.ndarray, decimals: int = 0) -> np.ndarray:
    return np.trunc(vals * (10**decimals)) / (10**decimals)


def no_rounding(vals):
    return vals


INSTANCE_FORMATS = ["vrplib", "solomon"]
ROUND_FUNCS: Dict[str, _RoundingFunc] = {
    "round": np.round,
    "round1": functools.partial(np.round, decimals=1),
    "round2": functools.partial(np.round, decimals=2),
    "trunc": trunc,
    "trunc1": functools.partial(trunc, decimals=1),
    "trunc2": functools.partial(trunc, decimals=2),
    "none": no_rounding,
}


def read(
    where: Union[str, pathlib.Path],
    instance_format: str = "vrplib",
    round_func: Union[str, _RoundingFunc] = no_rounding,
    scale: Optional[float] = None,
) -> ProblemData:
    """
    Reads the VRPLIB file at the given location, and returns a ProblemData
    instance.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        VRPLIB format.
    instance_format, optional
        File format of the instance to read, one of ``'vrplib'`` (default) or
        ``'solomon'``.
    round_func, optional
        Optional rounding function. Will be applied to round data if the data
        is not already integer. This can either be a function or a string:

            * ``'round'`` rounds the values to the nearest integer;
            * ``'round1'`` rounds the value to 1 decimal place;
            * ``'round2'`` rounds the value to 2 decimal places;
            * ``'trunc'`` truncates the values to be integral;
            * ``'trunc1'`` truncates the values to 1 decimal place;
            * ``'trunc2'`` truncates the values to 2 decimal place;
            * ``'dimacs'`` special value to set `scale` = 10 and truncate;
            * ``'none'`` does no rounding. This is the default.
    scale, optional
        Scale by which to multiply distances, durations and coordinates before
        rounding. By default, distances are not scaled.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """

    if round_func == "dimacs":
        if scale is not None and scale != 10:
            raise ValueError("Cannot use scale with 'dimacs' round func.")
        scale = 10
        round_func = "trunc"

    if isinstance(round_func, str) and round_func in ROUND_FUNCS:
        round_func = ROUND_FUNCS[round_func]
    elif not callable(round_func):
        raise ValueError(
            f"round_func = {round_func} is not understood. Can be a function,"
            f" or one of {ROUND_FUNCS.keys()}."
        )

    def apply_rounding(vals: np.ndarray) -> np.ndarray:
        if scale is not None:
            vals = vals * scale
        vals = round_func(vals)  # type: ignore
        return vals if PRECISION == "double" else _safe_convert_to_int(vals)

    instance = vrplib.read_instance(where, instance_format=instance_format)

    # A priori checks
    if "dimension" in instance:
        num_clients = instance["dimension"]
    else:
        if "demand" not in instance:
            raise ValueError("File should either contain dimension or demands")
        num_clients = len(instance["demand"])

    depots = instance.get("depot", np.array([0]))
    num_vehicles = instance.get("vehicles", num_clients - 1)
    capacity = instance.get("capacity", _INT_MAX)
    edge_weight = apply_rounding(instance["edge_weight"])

    if "demand" in instance:
        demands = instance["demand"]
    else:
        demands = np.zeros(num_clients, dtype=int)

    if "node_coord" in instance:
        coords = apply_rounding(instance["node_coord"])
    else:
        coords = np.zeros((num_clients, 2), dtype=int)

    if "service_time" in instance:
        service_times = apply_rounding(instance["service_time"])
    else:
        service_times = np.zeros(num_clients, dtype=int)

    if "time_window" in instance:
        time_windows = apply_rounding(instance["time_window"])
    else:
        # The default value for the latest time window based on the maximum
        # route duration. This ensures that the time window constraints are
        # always satisfied.
        bound = num_clients * (edge_weight.max() + service_times.max())
        bound = min(bound, _INT_MAX)
        time_windows = np.repeat([[0, bound]], num_clients, axis=0)

    # Checks
    if len(depots) != 1 or depots[0] != 0:
        raise ValueError(
            "Source file should contain single depot with index 1 "
            + "(depot index should be 0 after subtracting offset 1)"
        )

    if demands[0] != 0:
        raise ValueError("Demand of depot must be 0")

    if time_windows[0, 0] != 0:
        raise ValueError("Depot start of time window must be 0")

    if service_times[0] != 0:
        raise ValueError("Depot service duration must be 0")

    if (time_windows[:, 0] > time_windows[:, 1]).any():
        raise ValueError("Time window cannot start after end")

    return ProblemData(
        coords,
        demands,
        num_vehicles,
        capacity,
        time_windows,
        service_times,
        edge_weight,
    )


def read_solution(where: Union[str, pathlib.Path]) -> _Routes:
    """
    Reads a solution in VRPLIB format from file at the given location, and
    returns the routes contained in it.

    Parameters
    ----------
    where
        File location to read. Assumes the solution in the file on the given
        location is in VRPLIB solution format.

    Returns
    -------
    list
        List of routes, where each route is a list of client numbers.
    """
    sol = vrplib.read_solution(str(where))
    return sol["routes"]  # type: ignore
