.. module:: pyvrp
   :synopsis: PyVRP


PyVRP
=====

The top-level :mod:`pyvrp` module exposes several of PyVRP's core components.
These include the :class:`~pyvrp._pyvrp.ProblemData` class defining a VRP instance, the :class:`~pyvrp.IteratedLocalSearch.IteratedLocalSearch` class that implements the solver, and the :class:`~pyvrp._pyvrp.Solution` class representing VRP solutions.

A typical workflow involves defining your problem instance through the :class:`~pyvrp.Model.Model` interface, solving it with its :meth:`~pyvrp.Model.Model.solve` method, and inspecting the resulting :class:`~pyvrp.Result.Result` object.
This object stores the best observed solution and detailed runtime statistics.

.. hint::

   Have a look at the examples to see how these classes relate!

.. automodule:: pyvrp.Model

   .. autoclass:: Model
      :members:

   .. autoclass:: Edge
      :members:

   .. autoclass:: Profile
      :members:

.. automodule:: pyvrp.IteratedLocalSearch

   .. autoclass:: IteratedLocalSearchParams
      :members:

   .. autoclass:: IteratedLocalSearch
      :members:

.. automodule:: pyvrp.minimise_fleet
   :members:

.. automodule:: pyvrp.PenaltyManager

   .. autoclass:: PenaltyParams
      :members:

   .. autoclass:: PenaltyManager
      :members: 

.. automodule:: pyvrp.Population

   .. autoclass:: PopulationParams
      :members:

   .. autoclass:: Population
      :members:
      :special-members: __iter__, __len__

.. automodule:: pyvrp.read

   .. autofunction:: read

   .. autofunction:: read_solution

.. automodule:: pyvrp.Result
   :members:

.. automodule:: pyvrp.show_versions
   :members:

.. automodule:: pyvrp.solve

   .. autoclass:: SolveParams
      :members:

   .. autofunction:: solve

.. automodule:: pyvrp.Statistics
   :members:

.. automodule:: pyvrp._pyvrp

   .. autoclass:: CostEvaluator
      :members:

   .. autoclass:: Route
      :members:

   .. autoclass:: Solution
      :members:

   .. autoclass:: Trip
      :members:

   .. autoclass:: Client
      :members:

   .. autoclass:: ClientGroup
      :members:
      :special-members: __iter__, __len__

   .. autoclass:: Depot
      :members:

   .. autoclass:: VehicleType
      :members:

   .. autoclass:: ProblemData
      :members:

   .. autoclass:: ScheduledVisit
      :members:

   .. autoclass:: DynamicBitset
      :members:
      :special-members: __and__, __or__, __xor__, __getitem__, __setitem__,
                        __invert__, __len__, __eq__

   .. autoclass:: RandomNumberGenerator
      :members:
      :special-members: __call__

.. automodule:: pyvrp.exceptions

   .. autoexception:: ScalingWarning

   .. autoexception:: TspWarning

   .. autoexception:: PenaltyBoundWarning
