import pytest

from pyvrp import solve
from pyvrp.stop import MaxIterations


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
@pytest.mark.parametrize("iterations", [1, 10])
def test_solve_one(instance, iterations, benchmark, request):
    """
    Tests performance of solving various instances.
    """
    data = request.getfixturevalue(instance)
    benchmark(solve, data, stop=MaxIterations(iterations), seed=0)
