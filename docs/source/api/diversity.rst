.. module:: pyvrp.diversity
   :synopsis: Diversity measures


Diversity measures
==================

The :mod:`pyvrp.diversity` module contains operators that determine the relative difference between two :class:`~pyvrp._Individual.Individual` solutions.
This difference provides a measure of diversity: a :class:`~pyvrp.Population.Population` of highly diverse solutions allows the genetic algorithm to perform better.
At the same time, one also wants to balance diversity with quality: the solutions should also be good.
That balance is maintained by computing a fitness score for each solution, which balances the diversity with the objective value.

.. autoapimodule:: pyvrp.diversity._broken_pairs_distance

   .. autoapifunction:: broken_pairs_distance
