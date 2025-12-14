.. module:: pyvrp.search
   :synopsis: Search


Search methods
==============

The :mod:`pyvrp.search` module contains classes and search methods responsible for modifying or improving solutions.
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

.. automodule:: pyvrp.search._search

   .. autoclass:: LocalSearchStatistics

   .. autoclass:: PerturbationManager

.. automodule:: pyvrp.search.neighbourhood
   :members:


Node operators
--------------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_node_operator` method.
Each node operator inherits from :class:`~pyvrp.search._search.NodeOperator`.
As a convenience, the :mod:`pyvrp.search` module makes most relevant operators available as ``NODE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import NODE_OPERATORS


.. automodule:: pyvrp.search._search
   :noindex:

   .. autoclass:: NodeOperator

   .. autoclass:: Exchange10
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: Exchange20
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: Exchange30
      :exclude-members: evaluate, apply, statistics, supports
   
   .. autoclass:: Exchange11
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: Exchange21
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: Exchange31
      :exclude-members: evaluate, apply, statistics, supports
   
   .. autoclass:: Exchange22
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: Exchange32
      :exclude-members: evaluate, apply, statistics, supports
   
   .. autoclass:: Exchange33
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: RelocateWithDepot
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: SwapTails
      :exclude-members: evaluate, apply, statistics, supports


Route operators
---------------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_route_operator` method.
Each route operator inherits from :class:`~pyvrp.search._search.RouteOperator`.
As a convenience, the :mod:`pyvrp.search` module makes these operators available as ``ROUTE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import ROUTE_OPERATORS


.. automodule:: pyvrp.search._search
   :noindex:

   .. autoclass:: RouteOperator

   .. autoclass:: SwapRoutes
      :exclude-members: evaluate, apply, statistics, supports

   .. autoclass:: SwapStar
      :exclude-members: evaluate, apply, statistics, supports
