from matplotlib.testing.decorators import image_comparison as img_comp

from pyvrp import (
    CostEvaluator,
    Population,
    PopulationParams,
    Solution,
    XorShift128,
    plotting,
)
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import make_random_solutions, read, read_solution

IMG_KWARGS = dict(remove_text=True, tol=2, extensions=["png"], style="mpl20")


@img_comp(["plot_solution", "plot_solution_with_customers"], **IMG_KWARGS)
def test_plot_solution():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    bks = read_solution("data/RC208.sol")

    sol = Solution(data, bks)

    plotting.plot_solution(sol, data)
    plotting.plot_solution(sol, data, plot_clients=True)


@img_comp(["plot_result"], **IMG_KWARGS)
def test_plot_result():
    num_iterations = 100

    data = read("data/RC208.txt", "solomon", round_func="trunc")
    bks = read_solution("data/RC208.sol")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    params = PopulationParams()
    pop = Population(broken_pairs_distance, params=params)

    for sol in make_random_solutions(params.min_pop_size, data, rng):
        pop.add(sol, cost_evaluator)

    stats = Statistics()

    for i in range(num_iterations):
        if i == num_iterations // 2:
            # Make sure we insert a feasible solution
            sol = Solution(data, bks)
        else:
            sol = Solution.make_random(data, rng)

        pop.add(sol, cost_evaluator)
        stats.collect_from(pop, cost_evaluator)

        # Hacky to produce deterministic result
        stats.runtimes[-1] = i % 3

    res = Result(Solution(data, bks), stats, num_iterations, 0.0)
    plotting.plot_result(res, data)


@img_comp(["plot_instance"], **IMG_KWARGS)
def test_plot_instance():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    plotting.plot_instance(data)


@img_comp(["plot_route_schedule"], **IMG_KWARGS)
def test_plot_route_schedule():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    bks = read_solution("data/RC208.sol")
    sol = Solution(data, bks)
    plotting.plot_route_schedule(data, sol.get_routes()[0])
