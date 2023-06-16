An introduction to VRP
======================

The vehicle routing problem (VRP) is one of the most well-studied problem in the field of operations research, motivated by numerous industrial applications.
The overarching goal of the VRP is to determine a set of vehicle routes to fulfill all (or some) transportation requests at the lowest possible cost.
Motivated by the enormous potential for cost savings, designing efficient algorithms that can compute low cost solutions have been the central interest among VRP researchers.

(TODO: insert delivery nice image here?)

VRP variants
------------

Here, we introduce the VRP variants supported by PyVRP, along with providing a formal definition for each of these variations.
An extensive list VRPs can be found in the seminal work by `Toth and Vigo (2014) <https://doi.org/10.1137/1.9780898718515>`_.

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

The Vehicle Routing Problem with Time Windows (VRPTW) is a variant of the VRP that introduces timing constraints.
Each arc :math:`(i, j) \in A` have an additional weight :math:`t_{ij}`, denoting the travel time from client :math:`i` to :math:`j`.
Each customer :math:`i \in V_c` has a demand :math:`q_{i} \ge 0`, a service time :math:`s_{i} \ge 0` and a (hard) time window :math:`\left[e_i, l_i\right]` that denotes the earliest and latest time that service can start.
A vehicle is allowed to arrive at a customer location before the beginning of the time window, but it must wait for the window to open to start the delivery.
Each vehicle must return to the depot before the end of the depot time window :math:`H`.

A feasible solution to the VRPTW consists of a set of routes where each vehicle arrives at each customer location within the specified time window and returns to the depot in time.
The objective is to find a feasible solution that minimizes the overall travel cost, which is often defined as the total distance.

.. hint::
    Check out :ref:`this example notebook </examples/vrptw.ipynb>` in which we solve a VRPTW instance.


Prize-collecting vehicle routing problem
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the prize-collecting vehicle routing problem (PCVRP), ....

(TODO: also known as VRP with Profits)
TODO


Solving VRPs
------------
While finding a feasible solution for a given VRP is often relatively simple, discovering the optimal solution can be considerably more complex as most VRP variants are classified as `NP-hard <https://en.wikipedia.org/wiki/NP-hardness>`_ problems.
Various heuristics, metaheuristics, and exact methods have been developed to tackle the VRPTW, including but not limited to, local search, genetic algorithms, and branch-and-cut algorithms.

.. note::

    PyVRP primarily implements heuristic and metaheuristic algorithms for solving vehicle routing problems (VRPs). As these algorithms do not guarantee optimal solutions, we rigorously :doc:`benchmark <benchmarks>` them to evaluate their effectiveness. For an exact state-of-the-art solver, we refer to `VRPSolverEasy <https://github.com/inria-UFF/VRPSolverEasy>`_.
