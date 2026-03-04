import pytest

from pyvrp._pyvrp import _BUILD_TYPE

skip_if_release = pytest.mark.skipif(
    condition=_BUILD_TYPE == "RELEASE",
    reason="This cannot be tested with release builds.",
)

skip_if_debug = pytest.mark.skipif(
    condition=_BUILD_TYPE != "RELEASE",
    reason="This cannot be tested with debug(optimized) builds.",
)
