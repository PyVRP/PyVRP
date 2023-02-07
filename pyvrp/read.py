import functools
import pathlib
from typing import Callable, Optional, Union

import numpy as np
import vrplib

from .ProblemData import ProblemData


def convert_to_int(vals: np.ndarray):
    return vals.astype(int)


def scale_and_truncate_to_decimals(vals: np.ndarray, decimals: int = 0):
    return (vals * (10**decimals)).astype(int)


def no_rounding(vals):
    return vals


def read(
    where: Union[str, pathlib.Path],
    instance_format: str = "vrplib",
    round_func: Optional[Union[str, Callable]] = None,
) -> ProblemData:
    """
    Reads the (C)VRPLIB file at the given location, and returns a ProblemData
    instance.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        (C)VRPLIB format.
    instance_format, optional
        File format of the instance to read. Currently supported are 'vrplib'
        (default) and 'solomon'.
    round_func, optional
        Function to apply to distances/durations, node coordinates, time
        windows and service times to specify how to round to integer. Required
        if data is not already integer.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """
    _round_func = no_rounding
    if isinstance(round_func, str):
        _round_func = ROUND_FUNCS[round_func]
    elif isinstance(round_func, Callable):
        _round_func = round_func
    else:
        assert round_func is None

    instance = vrplib.read_instance(where, instance_format=instance_format)

    # The formats supported so far don't have release times
    release_times = np.zeros_like(instance["demand"])
    return ProblemData(
        _round_func(instance["node_coord"]),
        instance["demand"],
        instance["vehicles"],
        instance["capacity"],
        _round_func(instance["time_window"]),
        _round_func(instance["service_time"]),
        _round_func(instance["edge_weight"]),
        release_times,
    )


ROUND_FUNCS = {
    "trunc": convert_to_int,
    "trunc1": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "dimacs": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "none": no_rounding,
}
