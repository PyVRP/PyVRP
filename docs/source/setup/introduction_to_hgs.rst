A brief introduction to HGS for VRP
===================================


PyVRP provides a high-performance implementation of the hybrid genetic search (HGS) algorithm for vehicle routing problems (VRPs).
HGS combines the global search capabilities of genetic algorithms with the fine-tuning efficiency of local search methods.
This hybrid approach allows for effective exploration and exploitation of the search space, leading to high-quality solutions as shown in `Vidal et al. (2013) <https://www.sciencedirect.com/science/article/pii/S0305054812001645>`_

The HGS algorithm works as follows. In every iteration of the search loop, the genetic algorithm selects two existing solutions from the population, using a so-called *k*-ary tournament to ensure more promising individuals (in terms of quality and diversity) get selected more frequently. A crossover operator then takes two parent solutions and use those to generate an offspring solution that inherits features from both parents. After crossover completes, the offspring solution is improved using local search, and then added to the population. If the population reaches the maximal size, survivor selection is performed that reduces the population to its minimal size.

The algorithm continues until a provided stopping criterion is met, at which point it returns the best solution found. In pseudocode, HGS works as follows:

.. line-block::

    **Input:** initial solutions :math:`s_1, \dots, s_{n}`
    **Output:** the best found solution :math:`s^*`
    :math:`s^* \gets s`
    **repeat** until stopping criteria is met:
        Select two parent solutions :math:`(p_1, p_2)` from the population using :math:`k`-ary tournament.
        Apply crossover operator :math:`XO` to produce an offspring solution :math:`s^o=XO(p_1, p_2)`.
        Improve the offspring using a local search procedure :math:`LS` to obtain :math:`s^c=LS(s^o)`.

        **if** :math:`s^c` has a better objective value than :math:`s^*`:
            :math:`s^* \gets s^c`

        Add the candidate solution to the population.

        **if** population size exceeds maximum size:
            Purge the solutions with lowest fitness until population size is at minimum size

    **return** :math:`s^*`

The `pyvrp` package provides the HGS algorithm, crossover operators, stopping criteria, and numerous local search operators.
You only need provide the initial solutions, for which we suggest to take random initial solutions to ensure diversity in the search.

.. hint::
    See the examples :doc:`Vehicle Routing Problem with Time Windows <../examples/vrptw>` and :doc:`Capacitated Vehicle Routing Problem <../examples/cvrp>` on how to setup HGS to solve these VRPs.
