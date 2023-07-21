.. module:: pyvrp.search
   :synopsis: Search


Search
======

The :mod:`pyvrp.search` module contains classes and methods responsible for improving a newly created offspring solution.
This happens just after :mod:`pyvrp.crossover` is performed by the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`. 

.. automodule:: pyvrp.search.LocalSearch

   .. autoclass:: LocalSearch
      :members:

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
      :members:

   .. autoclass:: Exchange10
      :members:

   .. autoclass:: Exchange20
      :members:

   .. autoclass:: Exchange30
      :members:
   
   .. autoclass:: Exchange11
      :members:

   .. autoclass:: Exchange21
      :members:

   .. autoclass:: Exchange31
      :members:
   
   .. autoclass:: Exchange22
      :members:

   .. autoclass:: Exchange32
      :members:
   
   .. autoclass:: Exchange33
      :members:

   .. autoclass:: MoveTwoClientsReversed
      :members:

   .. autoclass:: TwoOpt
      :members:


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
      :members:

   .. autoclass:: RelocateStar
      :members:

   .. autoclass:: SwapStar
      :members:
