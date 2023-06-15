An introduction to VRP
======================

The Vehicle Routing Problem (VRP) is a widely studied challenge in the field of operations research, motivated by practical applications across industries such as parcel delivery services, the distribution goods from warehouses to retailers, and waste collection services.
The overarching goal of the VRP is to determine a set of vehicle routes to fulfill all (or some) transportation requests at the lowest possible cost, utilizing the available vehicle fleet and respecting operational constraints.

(TODO: insert delivery nice image here)

Motivated by the enormous potential for cost savings, designing algorithms that can compute efficient solutions have been the central interest among VRP researchers in the recent decades.
While finding a feasible solution for a given VRP may be relatively simple, discovering the optimal solution can be considerably more complex due to the combinatorial search space.
Many VRP variants are classified as NP-hard, suggesting that solving these problems optimally within polynomial time is unlikely.
Various heuristics, metaheuristics, and exact methods have been developed to tackle the VRPTW, including but not limited to, local search, tabu search, genetic algorithms, and branch-and-cut algorithms.

.. note::

    PyVRP focuses on the implementation of heuristic and metaheuristic algorithms to solve large-scale vehicle routing problems. Although not proven optimal, we benchmark our algorithm against the ...
    For an exact state-of-the-art solution method, we refer to `VRPSolverEasy <https://github.com/inria-UFF/VRPSolverEasy>`_.


VRP variants
---------------------------

We now present the VRP variants that are supported by PyVRP. These VRP variants are common in the academic nomenclature, and we also present a formal definition for each of these variants.
An extensive list can be found in the seminal work by `Toth and Vigo (2014) <https://doi.org/10.1137/1.9780898718515>`_.

.. note::

    Interested in variants that are not supported yet by PyVRP? Feel free to make an issue on `GitHub <https://github.com/PyVRP/PyVRP/issues>`_!

In the following, we consider a complete graph :math:`G=(V,A)`, where :math:`V` is the vertex set and :math:`A` is the arc set.
The vertex set :math:`V` is partitioned into :math:`V=\{0\} \cup V_c`, where :math:`0` represents the depot and :math:`V_c=\{1, \dots, n\}` denotes the set of :math:`n` customers.
Each arc :math:`(i, j) \in A` has a weight :math:`d_{ij} \ge 0` that represents the travel distance from :math:`i \in V` to :math:`j \in V`, which need not to be symmetric and/or euclidean.
The fleet of vehicles :math:`K = \{1, 2, \dots, |K| \}` is assumed to be available at the depot and operate at identical costs.

Capacitated vehicle routing problem
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most studied variant of the vehicle routing problem is the *capacitated vehicle routing problem* (CVRP).
In this variant, each customer :math:`i \in V_c` has a demand :math:`q_{i} \ge 0`.
It is also assumed that the fleet of vehicles :math:`K` is assumed to be homogeneous, meaning that they all have the same maximum capacity :math:`Q > 0`.

A feasible solution to the CVRP consists of a set of routes that all begin and end at the depot, such that each customer is visited exactly once and none of the vehicles exceed their capacity.
The objective is to find a feasible solution that minimizes the total distance.

Note that most vehicle routing problem variants are extensions of the CVRP.

.. hint::
    Check out :ref:`this example notebook </examples/cvrp.ipynb>` in which we solve a CVRP instance.

Vehicle routing problem with time windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Vehicle Routing Problem with Time Windows (VRPTW) is an extension which introduces timing constraints.

In the VRPTW, each customer is associated with a time window during which the delivery must occur.
The time window is defined as an interval, often denoted as [:math:`e_i`, :math:`l_i`], where :math:`e_i` represents the earliest time the customer can accept the delivery and :math:`l_i` represents the latest time the customer can accept the delivery.
Moreover, each customer has a service time :math:`s_i \ge 0`.
A vehicle is allowed to arrive at a customer location before the beginning of the time window, but it must wait for the window to open to start the delivery.
Each vehicle must return to the depot before the end of the depot time window :math:`H`.
A feasible solution to the VRPTW consists of a set of routes where each vehicle arrives at each customer location within the specified time window and returns to the depot in time. Additionally, the solution must ensure that each customer is visited exactly once.
The objective of the VRPTW is to find a feasible solution that minimizes the overall travel cost.

.. hint::
    Check out :ref:`this example notebook </examples/vrptw.ipynb>` in which we solve a VRPTW instance.


Prize-collecting vehicle routing problem
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the prize-collecting vehicle routing problem (PCVRP), ....
TODO
