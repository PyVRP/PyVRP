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
As a convenience, the :mod:`pyvrp.search` module makes all these operators available as ``NODE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import NODE_OPERATORS


.. automodule:: pyvrp.search._Exchange

   .. autoapiclass:: Exchange10
      :members:

   .. autoapiclass:: Exchange20
      :members:

   .. autoapiclass:: Exchange30
      :members:
   
   .. autoapiclass:: Exchange11
      :members:

   .. autoapiclass:: Exchange21
      :members:

   .. autoapiclass:: Exchange31
      :members:
   
   .. autoapiclass:: Exchange22
      :members:

   .. autoapiclass:: Exchange32
      :members:
   
   .. autoapiclass:: Exchange33
      :members:

.. automodule:: pyvrp.search._MoveTwoClientsReversed

   .. autoapiclass:: MoveTwoClientsReversed
      :members:

.. automodule:: pyvrp.search._TwoOpt

   .. autoapiclass:: TwoOpt
      :members:

Route operators
---------------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_route_operator` method.
As a convenience, the :mod:`pyvrp.search` module makes all these operators available as ``ROUTE_OPERATORS``:

.. code-block:: python

   from pyvrp.search import ROUTE_OPERATORS


.. automodule:: pyvrp.search._RelocateStar

   .. autoapiclass:: RelocateStar
      :members:

.. automodule:: pyvrp.search._SwapStar

   .. autoapiclass:: SwapStar
      :members:
