import pytest

from pyvrp import solve
from pyvrp.stop import MaxIterations


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_solve_one_iteration(instance, benchmark, request):
    """
    Tests performance of solving various instances using one iteration.
    """
    data = request.getfixturevalue(instance)
    benchmark(solve, data, stop=MaxIterations(1), seed=0)


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_solve_more_iterations(instance, benchmark, request):
    """
    Tests performance of solving various instances over several iterations.
    """
    data = request.getfixturevalue(instance)
    benchmark(solve, data, stop=MaxIterations(10), seed=0)
