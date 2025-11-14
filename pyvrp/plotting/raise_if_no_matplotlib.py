import importlib
from functools import wraps


def raise_if_no_matplotlib(fn):
    @wraps(fn)
    def wrapper(*args, **kwargs):
        try:
            importlib.import_module("matplotlib")
        except ImportError:
            msg = """
                Using PyVRP's plotting functionality requires matplotlib.
                Please install pyvrp[plot] to use this functionality.
            """
            raise ImportError(msg) from None

        return fn(*args, **kwargs)

    return wrapper
