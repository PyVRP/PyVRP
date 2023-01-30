import pathlib
from functools import lru_cache

from pyvrp.read import read as _read


@lru_cache
def read(where: str):
    """
    Lightweight wrapper around ``pyvrp.read()``, reading files relative to the
    current directory.
    """
    this_dir = pathlib.Path(__file__).parent
    return _read(this_dir / where)
