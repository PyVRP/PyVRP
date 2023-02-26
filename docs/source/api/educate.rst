.. module:: pyvrp.educate
   :synopsis: Education


Education
=========

The :mod:`pyvrp.educate` module contains classes and methods responsible for educating (improving) a newly created offspring solution.
This happens just after :mod:`pyvrp.crossover` is performed by the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`. 

.. automodule:: pyvrp.educate.LocalSearch

   .. autoclass:: LocalSearch
      :members:

.. autoapimodule:: pyvrp.educate._Exchange

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

.. automodule:: pyvrp.educate.neighbourhood
   :members:

.. autoapimodule:: pyvrp.educate._MoveTwoClientsReversed

   .. autoclass:: MoveTwoClientsReversed
      :members:

.. autoapimodule:: pyvrp.educate._RelocateStar

   .. autoclass:: RelocateStar
      :members:

.. autoapimodule:: pyvrp.educate._SwapStar

   .. autoclass:: SwapStar
      :members:

.. autoapimodule:: pyvrp.educate._TwoOpt

   .. autoclass:: TwoOpt
      :members:
