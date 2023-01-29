import pathlib
from typing import Union

from pyvrp._lib.hgspy import ProblemData


def read(where: Union[str, pathlib.Path]) -> ProblemData:
    """
    Reads the (C)VRPLIB file at the given location, and returns a ProblemData
    instance.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        (C)VRPLIB format.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """
    # TODO move file IO from C++ to Python
    return ProblemData.from_file(str(where))
