Supporting new VRP variants
===========================

On this page we describe some guidelines and 'gotchas' to think about while adding support for new VRP variants to PyVRP.
These are given as hints.
To make these guidelines concrete, we take examples from our experience adding support for prize-collecting VRPs, which was added to PyVRP in version `0.3.0 <https://github.com/PyVRP/PyVRP/tree/4632ce97cedbc9d58216c2bec43cd679eb1d21c9>`_.
For reference, the pull request implementing prize-collecting is available `here <https://github.com/PyVRP/PyVRP/pull/213>`_.

.. note::

   If you want to merge your work into PyVRP itself, please first open an issue in the repository.
   It is also recommended to open an early draft pull request.
   How to set everything up for development is documented on the :doc:`contributing <contributing>` page.

Adding support for new VRP variants starts with thinking about where to add the new problem data needed to solve it.
In the case of prize-collecting, this meant adding the ``prize: int`` and ``required: bool`` fields to the :class:`~pyvrp._pyvrp.Client` object:

* The ``prize`` field indicates the prize obtained for visiting the client;
* The ``required`` field indicates whether the client *must* be part of a feasible solution, regardless of the prize.

Adding such data requires modifying the :class:`~pyvrp._pyvrp.Client` class, which is written in C++.
After changing C++ functionality, don't forget to update the Python bindings and type stub files with the new changes!

.. hint::

   When adding or modifying C++ functionality, don't forget to update the Python bindings and type stub files!

When adding new data fields, it is particularly important to think about sane default values.
For example, the standard CVRP and VRPTW do not consider prizes, and all clients must be visited in those problem settings.
It thus makes sense to make prize-collecting an *opt-in* feature, with defaults ``prize = 0`` and ``required = True``.

.. hint::

   Think about sane default values for any new data fields!

With the required problem data now available, the next step is to think about what needs to change in terms of the objective, and how to support its evaluation.
This is not always required: for example, if your VRP variant only introduces additional constraints, you can typically skip this step.
In the case of prize-collecting, however, the prizes must be added to the objective.

This requires modifying the :class:`~pyvrp._pyvrp.Solution`'s and :class:`~pyvrp._pyvrp.CostEvaluator`'s implementation of the cost components.
In particular, we updated the :class:`~pyvrp._pyvrp.Solution` to also compute the total value of all *uncollected* prizes, and changed :meth:`~pyvrp._pyvrp.CostEvaluator.cost` to compute

.. math::

   \overbrace{\sum_{(i, j)} d_{ij} x_{ij}}^{\text{distance}} + \overbrace{\sum_{i} p_i (1 - y_i)}^{\text{uncollected prizes}},

where :math:`x_{ij} \in \{0, 1\}` indicates if arc :math:`(i, j)` is used, and :math:`y_i \in \{0, 1\}` indicates if client :math:`i` is visited.

.. hint::

   Think about whether you need to modify the objective function.
   If you do, a good starting point is to look at the implementations of the :class:`~pyvrp._pyvrp.Solution` and the :class:`~pyvrp._pyvrp.CostEvaluator`.

The next step is to modify the search methods in :mod:`pyvrp.search` to respect the constraints of the new VRP variant.
Depending on the VRP variant in question, this might be simple, or it could be a lot of work: if you are unsure whether your VRP variant will be a lot of work to support, please consider opening an issue for discussion first.

.. hint::

   Open an issue if you are unsure about how to proceed implementing your VRP variant.

In the :mod:`pyvrp.search` module, either changes need to be made to the operators, or the :class:`~pyvrp.search.LocalSearch` object.
In the case of prize-collecting, it was the latter: we added support for evaluating (and applying) moves that inserted a client into the solution, or removed a client from it.
The required evaluation logic was easy to write by looking at the implementation of :class:`~pyvrp.search._search.Exchange10`.

With those changes in place, a basic implementation supporting the new VRP variant is typically already functional.
This is more than sufficient for an initial patch, so please open a pull request around this time.
To get that pull request merged, two more things are required:

* Tests exercising the new or modified code.
  These tests should check edge cases that are not supported (are errors raised when they should?), and ensure the basic functionality is correct.
* Benchmark results, on existing benchmark instances (see :doc:`benchmarking <benchmarking>` for an explanation of how we benchmark), and possibly on benchmark instances for the new VRP variant.
  Benchmarks on existing instances ensures the new code does not cause a performance regression, and benchmarks on new instances ensures we have a baseline for the newly supported VRP variant.
  New benchmark instances are not strictly necessary if the changes are very small (e.g., only adding a single new constraint) - this will be decided on a case-by-case basis during review of your pull request.

.. hint::

   A successful pull request adds tests and shows benchmark results!


With a basic implementation in place, PyVRP should now be able to find solutions for the new VRP variant.
Of course, it might be possible to further improve the implementation.
In the case of prize-collecting, after the initial implementation, we also:

* Modified the computation of the granular neighbourhood in :func:`~pyvrp.search.neighbourhood.compute_neighbours` to take prizes into account.
* Updated various statistics to display the number of clients in a solution.
* Changed :meth:`~pyvrp._pyvrp.Solution.neighbours` to return ``None`` in case a client is *not* in the solution.

Such changes may come about later, as we further improve support for a new VRP variant: the pull request adding initial support should ideally be kept as simple as possible.

.. hint::

   Keep it simple.
   It is always possible to further improve support for a VRP variant in later pull requests.

We hope that the guidelines on this page will prove helpful when adding support for a new VRP variant.

.. note::

   For further inspiration, you may want to look at the pull requests that added:

   * Support for `client release times <https://github.com/PyVRP/PyVRP/pull/254>`_.
   * Support for `multiple vehicle types <https://github.com/PyVRP/PyVRP/pull/245>`_.
   * Support for `multiple depots <https://github.com/PyVRP/PyVRP/pull/411>`_.
