import pytest

from pyvrp import solve
from pyvrp.stop import MaxIterations


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_solve(instance, benchmark, request):
    """
    Tests performance of solving various instances.
    """
    data = request.getfixturevalue(instance)
    benchmark(solve, data, stop=MaxIterations(1), seed=0)
