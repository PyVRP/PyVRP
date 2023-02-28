.. module:: pyvrp.educate
   :synopsis: Education


Education
=========

The :mod:`pyvrp.educate` module contains classes and methods responsible for educating (improving) a newly created offspring solution.
This happens just after :mod:`pyvrp.crossover` is performed by the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`. 

.. automodule:: pyvrp.educate.LocalSearch

   .. autoclass:: LocalSearch
      :members:

.. automodule:: pyvrp.educate.neighbourhood
   :members:

Node operators
--------------

Instances of these operators can be added to the :class:`~pyvrp.educate.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.educate.LocalSearch.LocalSearch.add_node_operator` method.

.. automodule:: pyvrp.educate._Exchange

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

.. automodule:: pyvrp.educate._MoveTwoClientsReversed

   .. autoapiclass:: MoveTwoClientsReversed
      :members:

.. automodule:: pyvrp.educate._TwoOpt

   .. autoapiclass:: TwoOpt
      :members:

Route operators
---------------

Instances of these operators can be added to the :class:`~pyvrp.educate.LocalSearch.LocalSearch` object via the :meth:`~pyvrp.educate.LocalSearch.LocalSearch.add_route_operator` method.

.. automodule:: pyvrp.educate._RelocateStar

   .. autoapiclass:: RelocateStar
      :members:

.. automodule:: pyvrp.educate._SwapStar

   .. autoapiclass:: SwapStar
      :members:
