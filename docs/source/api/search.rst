.. module:: pyvrp.search
   :synopsis: Search


Search methods
==============

The :mod:`pyvrp.search` module contains classes and search methods responsible for improving a newly created offspring solution.
This happens just after :mod:`pyvrp.crossover` is performed by the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`. 
PyVRP currently provides a :class:`LocalSearch` method.

All search methods implement the :class:`SearchMethod` protocol.

.. automodule:: pyvrp.search.SearchMethod

   .. autoclass:: SearchMethod
      :members:
      :special-members: __call___

.. automodule:: pyvrp.search.LocalSearch

   .. autoclass:: LocalSearch
      :members:
      :special-members: __call___

.. automodule:: pyvrp.search.neighbourhood
   :members:


Node operators
--------------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_node_operator` method.
Each node operator inherits from :class:`~pyvrp.search._search.NodeOperator`.
As a convenience, the :mod:`pyvrp.search` module makes all these operators available as ``NODE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import NODE_OPERATORS


.. automodule:: pyvrp.search._search

   .. autoclass:: NodeOperator

   .. autoclass:: Exchange10
      :exclude-members: evaluate, apply

   .. autoclass:: Exchange20
      :exclude-members: evaluate, apply

   .. autoclass:: Exchange30
      :exclude-members: evaluate, apply
   
   .. autoclass:: Exchange11
      :exclude-members: evaluate, apply

   .. autoclass:: Exchange21
      :exclude-members: evaluate, apply

   .. autoclass:: Exchange31
      :exclude-members: evaluate, apply
   
   .. autoclass:: Exchange22
      :exclude-members: evaluate, apply

   .. autoclass:: Exchange32
      :exclude-members: evaluate, apply
   
   .. autoclass:: Exchange33
      :exclude-members: evaluate, apply

   .. autoclass:: MoveTwoClientsReversed
      :exclude-members: evaluate, apply

   .. autoclass:: TwoOpt
      :exclude-members: evaluate, apply


Route operators
---------------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_route_operator` method.
Each route operator inherits from :class:`~pyvrp.search._search.RouteOperator`.
As a convenience, the :mod:`pyvrp.search` module makes all these operators available as ``ROUTE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import ROUTE_OPERATORS


.. automodule:: pyvrp.search._search
   :noindex:

   .. autoclass:: RouteOperator

   .. autoclass:: RelocateStar
      :exclude-members: evaluate, apply

   .. autoclass:: SwapRoutes
      :exclude-members: evaluate, apply

   .. autoclass:: SwapStar
      :exclude-members: evaluate, apply
