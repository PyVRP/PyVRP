from numpy.testing import assert_, assert_equal

import pyvrp
from pyvrp import Activity, Client, CostEvaluator, RandomNumberGenerator
from pyvrp.search import compute_neighbours
from pyvrp.search._search import SearchSpace, Solution


def test_load_unload(ok_small):
    """
    Tests that loading and then unloading an unchanged solution returns the
    same original solution.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[0, 1], [2, 3]])

    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)
    assert_equal(search_sol.unload(), pyvrp_sol)


def test_loading_twice_in_a_row(ok_small):
    """
    When loading the same solution we re-use the representation of the previous
    solution as much as possible. Here we test that loading the same solution
    twice in a row (which triggers that re-use) still produces the same
    solution when unloading.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[0, 1], [2, 3]])

    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)
    search_sol.load(pyvrp_sol)
    assert_equal(search_sol.unload(), pyvrp_sol)


def test_clients_routes_access(ok_small):
    """
    Tests clients and routes access.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[0, 1], [2, 3]])
    search_sol = Solution(ok_small)
    search_sol.load(pyvrp_sol)

    # There should be #clients clients, and #vehicles routes.
    assert_equal(len(search_sol.clients), ok_small.num_clients)
    assert_equal(len(search_sol.routes), ok_small.num_vehicles)

    for idx in [0, 1]:  # [0, 1] are in the first route
        assert_equal(search_sol.clients[idx].activity, Activity(f"C{idx}"))
        assert_equal(search_sol.clients[idx].route, search_sol.routes[0])

    for idx in [2, 3]:  # [2, 3] are in the second route
        assert_equal(search_sol.clients[idx].activity, Activity(f"C{idx}"))
        assert_equal(search_sol.clients[idx].route, search_sol.routes[1])


def test_insert_required(ok_small):
    """
    Tests that inserting clients can fail if inserting is too expensive and
    the insert is not required.
    """
    data = ok_small
    search_space = SearchSpace(data, compute_neighbours(data))
    cost_eval = CostEvaluator([0], 0, 0)

    # Start with an empty solution and try to insert the first client without
    # requiring an insert. Inserting should fail: it's not worth it, since the
    # client has no prize. However, inserting should succeed when required.
    sol = Solution(data)
    assert_(not sol.insert(sol.clients[1], search_space, cost_eval, False))
    assert_(sol.insert(sol.clients[1], search_space, cost_eval, True))


def test_load_unload_shipments(small_shipments):
    """
    Tests loading and unloading a solution for an instance with shipments.
    """
    sol = Solution(small_shipments)
    assert_equal(len(sol.clients), small_shipments.num_clients)
    assert_equal(len(sol.shipments), small_shipments.num_shipments)

    # The solution stores shipment nodes as (pickup, delivery) pairs.
    pickup, delivery = sol.shipments[0]
    assert_equal(str(pickup), "L0")
    assert_equal(str(delivery), "U0")

    rng = RandomNumberGenerator(seed=42)
    pyvrp_sol = pyvrp.Solution.make_random(small_shipments, rng)

    # Let's test if loading and unloading results in the same solution.
    sol.load(pyvrp_sol)
    assert_equal(sol.unload(), pyvrp_sol)


def test_insert_shipment(small_shipments):
    """
    Tests inserting a shipment into an empty solution.
    """
    sol = Solution(small_shipments)

    pickup, delivery = sol.shipments[0]
    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)

    # This shipment does not offer a prize, so when inserting is not required,
    # there is no incentive to do so. Only when we force an insert is the
    # shipment actually inserted.
    assert_(not sol.insert(pickup, delivery, search_space, cost_eval, False))
    assert_(sol.insert(pickup, delivery, search_space, cost_eval, True))
    assert_(pickup.route and delivery.route)
    pickup.route.update()

    # First route contains the first shipment, the second route remains empty.
    assert_equal(str(sol), """Route #1: L0 U0\nRoute #2: \n""")


def test_insert_mixed_client_and_shipment(small_shipments):
    """
    Tests inserting a shipment and a client in a mixed instance.
    """
    data = small_shipments.replace(clients=[Client(location=2, delivery=[0])])
    sol = Solution(data)

    neighbours = compute_neighbours(data)
    search_space = SearchSpace(data, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)

    pickup, delivery = sol.shipments[1]  # first insert the second shipment
    assert_(sol.insert(pickup, delivery, search_space, cost_eval, True))
    assert_(pickup.route and delivery.route)
    pickup.route.update()

    client = sol.clients[0]  # now insert the only client
    assert_(not sol.insert(client, search_space, cost_eval, False))
    assert_(sol.insert(client, search_space, cost_eval, True))
    assert_(client.route)
    client.route.update()

    # C0 is much closer to U1 than to L1, so it is inserted after U1.
    assert_equal(str(sol.routes[0]), "L1 U1 C0")


def test_getitem(ok_small, small_shipments):
    """
    Tests getting a client or shipment node by indexing the solution.
    """
    sol = Solution(ok_small)

    # Indexing a client activity should return the associated client node.
    client = sol[Activity("C1")]
    assert_(client is sol.clients[1])
    assert_(client.is_client())
    assert_equal(client.idx, 1)

    # Depots are not tracked at the solution level, and indexing them returns
    # a null pointer/None.
    depot = sol[Activity("D0")]
    assert_(depot is None)

    sol = Solution(small_shipments)

    # Indexing a pickup activity returns the associated pickup node.
    pickup = sol[Activity("L2")]
    assert_(pickup is sol.shipments[2][0])
    assert_(pickup.is_pickup())
    assert_equal(pickup.idx, 2)

    # And likewise for delivery activities.
    delivery = sol[Activity("U3")]
    assert_(delivery is sol.shipments[3][1])
    assert_(delivery.is_delivery())
    assert_equal(delivery.idx, 3)


def test_insert_pickup_delivery_non_adjacent(small_shipments):
    """
    Tests that insert() can insert the pickup and delivery nodes in
    non-adjacent places.
    """
    sol = Solution(small_shipments)

    route = sol.routes[0]
    for descr in ["L0", "L1", "L3", "U0", "U1", "U3"]:
        node = sol[Activity(descr)]
        route.append(node)
    route.update()

    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)
    pickup, delivery = sol.shipments[2]

    sol.insert(pickup, delivery, search_space, cost_eval, True)
    route.update()

    # The solution should have inserted L2 and U2, but not next to each other.
    assert_equal(str(route), "L0 L1 L3 U0 L2 U1 U3 U2")


def test_insert_shipment_at_neighbour_predecessor(small_shipments):
    """
    Tests inserting a shipment at the predecessor of a neighbour.
    """
    sol = Solution(small_shipments)

    # Start with shipment 1 in the route. Shipment 0 remains unplanned.
    route = sol.routes[0]
    for descr in ["L1", "U1"]:
        route.append(sol[Activity(descr)])
    route.update()
    assert_equal(route.distance(), 27_732)

    # L1 is the only neighbour of shipment 0. Inserting shipment 0 should
    # therefore consider positions after L1 and after its predecessor, which
    # is the start depot.
    neighbours = {
        Activity(f"L{idx}"): [] for idx in range(small_shipments.num_shipments)
    }
    neighbours[Activity("L0")] = [Activity("L1")]
    search_space = SearchSpace(small_shipments, neighbours)

    pickup, delivery = sol.shipments[0]
    cost_eval = CostEvaluator([0], 0, 0)
    assert_(sol.insert(pickup, delivery, search_space, cost_eval, True))
    route.update()

    # Inserting after the start depot adds 3_125 distance, compared to 9_571
    # for opening the empty second route. Considering only positions after L1
    # would instead produce L1 L0 U1 U0.
    assert_equal(route.distance(), 30_857)
    assert_equal(str(route), "L0 U0 L1 U1")
