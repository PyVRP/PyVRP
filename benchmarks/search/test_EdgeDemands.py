import numpy as np
import pytest

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp.search import (
    Exchange11,
    LocalSearch,
    PerturbationManager,
    PerturbationParams,
)

NUM_CLIENTS = 90
SEED = 42


def _distance_matrix(size: int) -> np.ndarray:
    idx = np.arange(size, dtype=np.int64)
    mat = np.abs(idx[:, None] - idx[None, :]) + 1
    np.fill_diagonal(mat, 0)
    return mat


def _edge_demand_matrix(size: int) -> np.ndarray:
    frm = np.arange(size, dtype=np.int64)[:, None]
    to = np.arange(size, dtype=np.int64)[None, :]

    # Deterministic directed pattern with non-zero off-diagonals.
    mat = ((frm * 17 + to * 31) % 5 == 0).astype(np.int64)
    np.fill_diagonal(mat, 0)
    return mat


def _make_data(with_edge_demands: bool) -> ProblemData:
    size = NUM_CLIENTS + 1  # includes depot

    locs = [Location(idx, idx % 13) for idx in range(size)]
    clients = [Client(client, delivery=[1]) for client in range(1, size)]
    depots = [Depot(0)]

    # Keep capacity large so edge demands do not change objective values.
    veh_types = [VehicleType(num_available=3, capacity=[1_000_000])]
    dist = _distance_matrix(size)
    dur = dist.copy()

    try:
        if with_edge_demands:
            return ProblemData(
                locations=locs,
                clients=clients,
                depots=depots,
                vehicle_types=veh_types,
                distance_matrices=[dist],
                duration_matrices=[dur],
                edge_demand_matrices=[[_edge_demand_matrix(size)]],
            )

        return ProblemData(
            locations=locs,
            clients=clients,
            depots=depots,
            vehicle_types=veh_types,
            distance_matrices=[dist],
            duration_matrices=[dur],
        )
    except TypeError:
        if with_edge_demands:
            pytest.skip(
                "This PyVRP build does not support edge_demand_matrices yet."
            )
        raise


def _make_search(data: ProblemData):
    rng = RandomNumberGenerator(seed=SEED)
    neighbours = [
        [nb for nb in range(data.num_clients) if nb != client]
        for client in range(data.num_clients)
    ]
    ls = LocalSearch(
        data,
        rng,
        neighbours,
        PerturbationManager(PerturbationParams(0, 0)),
    )
    ls.add_operator(Exchange11(data))

    # Multiple routes improve move opportunities while keeping deterministic
    # route structure across benchmark rounds.
    routes = [
        list(range(0, 30)),
        list(range(30, 60)),
        list(range(60, data.num_clients)),
    ]
    sol = Solution(data, routes)
    cost_eval = CostEvaluator([1], 0, 0)

    return ls, sol, cost_eval


@pytest.mark.parametrize(
    "with_edge_demands",
    [False, True],
    ids=["without_edge_demands", "with_edge_demands"],
)
def test_exchange11_edge_demand_overhead(with_edge_demands, benchmark):
    """
    Benchmarks local-search runtime with and without edge demand matrices.

    For legacy comparison, run this benchmark on ``main``:
    ``without_edge_demands`` there is case (1), and on this branch it is case
    (2). Case (3) is ``with_edge_demands`` on this branch.
    """
    data = _make_data(with_edge_demands)
    ls, sol, cost_eval = _make_search(data)
    benchmark(ls, sol, cost_eval, exhaustive=True)
