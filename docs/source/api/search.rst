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

   .. autoclass:: PerturbationParams
      :members:

   .. autoclass:: PerturbationManager
      :members:
      :exclude-members: perturb

.. automodule:: pyvrp.search.neighbourhood
   :members:


Operators
---------

Instances of these operators can be added to the :class:`~pyvrp.search.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.search.LocalSearch.LocalSearch.add_operator` method.
Each operator inherits from :class:`~pyvrp.search._search.BinaryOperator` or :class:`~pyvrp.search._search.UnaryOperator`.
As a convenience, the :mod:`pyvrp.search` module makes most relevant operators available as ``OPERATORS``:

.. code-block:: python

   from pyvrp.search import OPERATORS


.. automodule:: pyvrp.search._search
   :noindex:

   .. autoclass:: BinaryOperator

   .. autoclass:: UnaryOperator

   .. autoclass:: Exchange10
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: Exchange20
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: Exchange30
      :exclude-members: evaluate, apply, statistics, supports, init
   
   .. autoclass:: Exchange11
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: Exchange21
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: Exchange31
      :exclude-members: evaluate, apply, statistics, supports, init
   
   .. autoclass:: Exchange22
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: Exchange32
      :exclude-members: evaluate, apply, statistics, supports, init
   
   .. autoclass:: Exchange33
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: RelocateWithDepot
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: RemoveAdjacentDepot
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: RemoveOptional
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: InsertOptional
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: ReplaceGroup
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: ReplaceOptional
      :exclude-members: evaluate, apply, statistics, supports, init

   .. autoclass:: SwapTails
      :exclude-members: evaluate, apply, statistics, supports, init
