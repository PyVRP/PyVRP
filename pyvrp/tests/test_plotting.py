from matplotlib.testing.decorators import image_comparison

from pyvrp import Individual, PenaltyManager, Population, XorShift128, plotting
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp.diversity import broken_pairs_distance
from pyvrp.helpers import make_individual
from pyvrp.tests.helpers import read, read_solution


@image_comparison(
    baseline_images=["plot_solution", "plot_solution_with_customers"],
    remove_text=True,
    extensions=["png"],
    style="mpl20",
)
def test_plot_solution():
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )
    solution = read_solution("data/RC208.sol")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = make_individual(data, pm, solution["routes"])

    plotting.plot_solution(indiv, data)
    plotting.plot_solution(indiv, data, plot_customers=True)


@image_comparison(
    baseline_images=["plot_result"],
    remove_text=True,
    extensions=["png"],
    style="mpl20",
)
def test_plot_result(num_iterations=100):
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )
    solution = read_solution("data/RC208.sol")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)
    pop = Population(data, pm, rng, broken_pairs_distance)
    stats = Statistics()

    for i in range(num_iterations):
        if i == num_iterations // 2:
            # Make sure we insert a feasible solution
            individual = make_individual(data, pm, solution["routes"])
        else:
            individual = Individual(data, pm, rng)

        pop.add(individual)
        stats.collect_from(pop)
        # Hacky to produce determinstic result
        stats.runtimes[-1] = i % 3

    res = Result(pop.get_best_found(), stats, num_iterations, 0.0)

    plotting.plot_result(res, data)


@image_comparison(
    baseline_images=["plot_instance"],
    remove_text=True,
    extensions=["png"],
    style="mpl20",
)
def test_plot_instance():
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )
    plotting.plot_instance(data)


@image_comparison(
    baseline_images=["plot_route_schedule"],
    remove_text=True,
    extensions=["png"],
    style="mpl20",
)
def test_plot_route_schedule(num_iterations=100):
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )
    solution = read_solution("data/RC208.sol")
    plotting.plot_route_schedule(data, solution["routes"][0])
