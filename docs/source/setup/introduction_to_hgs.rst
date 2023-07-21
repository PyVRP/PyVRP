A brief introduction to HGS
===========================

PyVRP provides a high-performance implementation of the hybrid genetic search (HGS) algorithm for vehicle routing problems (VRPs).
HGS combines the global search capabilities of genetic algorithms with the fine-tuning efficiency of (local) search methods.
This hybrid approach allows for effective exploration and exploitation of the search space, leading to high-quality solutions.

.. note::

   For a more thorough introduction to HGS for VRPs, we refer to the papers by `Vidal et al. (2013) <https://www.sciencedirect.com/science/article/pii/S0305054812001645>`_ and `Vidal (2022) <https://www.sciencedirect.com/science/article/pii/S030505482100349X>`_.

The HGS algorithm works as follows.
HGS maintains a population of solutions, which is initialised by a set of initial solutions given as input to the algorithm.
In every iteration of the search loop, the algorithm selects two existing parent solutions from the population using a *k*-ary tournament, favouring solutions with higher fitness.
A crossover operator then uses these two parent solutions to generate an offspring solution that inherits features from both parents.
After the crossover, the offspring solution is further improved using a search procedure.
The resulting candidate solution is first added to the population.
If the candidate solution improves over the best solution found so far, it is also registered as the new best solution.
Upon reaching the maximum population size, a survivor selection mechanism removes the least fit solutions until the population is back at the minimum size.
The algorithm continues until a provided stopping criterion is met, at which point it returns the best solution found. 

In pseudocode, HGS works as follows:

| **Input:** initial solutions :math:`s_1, \dots, s_{n}`
| **Output:** the best-found solution :math:`s^*`
| Set :math:`s^*` to the initial solution with the best objective value
| **repeat** until stopping criterion is met:
|     Select two parent solutions :math:`(s^{p_1}, s^{p_2})` from the population using :math:`k`-ary tournament.
|     Apply crossover operator :math:`XO` to generate an offspring solution :math:`s^o=XO(s^{p_1}, s^{p_2})`.
|     Improve the offspring using a search procedure :math:`LS` to obtain :math:`s^c=LS(s^o)`.
|     Add the candidate solution to the population.
|     **if** :math:`s^c` has a better objective value than :math:`s^*`:
|         :math:`s^* \gets s^c`
|     **if** population size exceeds maximum size:
|         Remove the solutions with the lowest fitness until the population is at minimum size
| **return** :math:`s^*`

PyVRP provides the HGS algorithm, crossover operators, stopping criteria, and various search procedures and operators.
You only need to provide the initial solutions, for which we suggest taking random initial solutions to ensure diversity in the search.

.. hint::

   See the :doc:`tutorial <../examples/quick_tutorial>` page to get started with PyVRP.
