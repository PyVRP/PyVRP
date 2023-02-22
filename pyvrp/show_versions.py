import sys
from importlib.metadata import version


def show_versions():
    """
    This function prints version information that is useful when filing bug
    reports.

    Examples
    --------
    Calling this function should print information like the following
    (dependency versions in your local installation will likely differ):

    >>> import pyvrp
    >>> pyvrp.show_versions()
    INSTALLED VERSIONS
    ------------------
         pyvrp: 1.0.0
         numpy: 1.23.4
    matplotlib: 3.5.1
        Python: 3.9.9
    """
    python_version = ".".join(map(str, sys.version_info[:3]))

    print("INSTALLED VERSIONS")
    print("------------------")
    print(f"     pyvrp: {version('pyvrp')}")
    print(f"     numpy: {version('numpy')}")
    print(f"matplotlib: {version('matplotlib')}")
    print(f"    Python: {python_version}")
