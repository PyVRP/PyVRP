A brief introduction to HGS for VRP
===================================

The PyVRP library provides an implementation of the Hybrid Genetic Search (HGS) algorithm largely based on `Vidal et al. (2013) <https://www.sciencedirect.com/science/article/pii/S0305054812001645>`_.
HGS combines the global search capabilities of genetic algorithms with the fine-tuning efficiency of local search methods.
This hybrid approach allows for effective exploration and exploitation of the search space, leading to high-quality solutions.

The HGS algorithm can be broken down into several key components:
- **Initialization**: The algorithm starts with a randomly generated initial population of solutions (routes) for the given VRP, which are further divided into feasible and infeasible sub-populations.
- **Offspring generation**: The HGS algorithm employs a combination of selection and crossover operators to generate new offspring solutions. The selection process ensures that fitter individuals have a higher probability of being chosen as parents for the next generation. The crossover operator combines features of parent solutions.
- **Local search**: To enhance the quality of solutions, the algorithm applies a local search method, such as 2-opt or 3-opt, to each offspring solution. These methods involve swapping or relocating nodes within a route to improve its overall cost. By incorporating local search, the HGS algorithm can effectively explore the solution space while also refining the solutions it generates.
- **Replacement**: A replacement strategy is used to maintain diversity in the population and ensure the search progresses towards better solutions. Offspring solutions are compared to their parents, and the better individuals are retained in the population. This process allows the algorithm to maintain a balance between exploration and exploitation. If the population size surpasses its maximum limit, it is reduced to a minimum size by favoring solutions with the highest biased fitness.

The algorithm continues until a provided stopping criterion is met, at which point it returns the best solution found. In pseudocode, HGS works as follows:

.. line-block::

    *****Input:** initial solutions :math:`s_1, \dots, s_{n}`
    **Output:** the best found solution :math:`s^*`
    :math:`s^* \gets s`
    **repeat** until stopping criteria is met:
        Select two parent solutions :math:`(p_1, p_2)` from the population using :math:`k`-ary tournament.
        Apply an crossover operator :math:`XO` to produce an offspring solution :math:`s^o=XO(p_1, p_2)`.
        Use a local search procedure :math:`LS` to improve the offspring :math:`s^c=LS(s^o)`.

        **if** :math:`s^c` has a better objective value than :math:`s^*`:
            :math:`s^* \gets s^c`

        Add the candidate solution to the population.

        **if** population size exceeds maximum size:
            purge the solutions with lowest biased fitness until population size is minimum

    **return** :math:`s^*`

The `pyvrp` package provides the HGS algorithm, crossover operators, stopping criteria, and numerous local search operators.
You only need provide the initial solutions, for which we suggest to take random initial solutions to ensure diversity in the search.

.. hint::
    See the examples :doc:`Vehicle Routing Problem with Time Windows <../examples/vrptw>` and :doc:`Capacitated Vehicle Routing Problem <../examples/cvrp>` on how to setup HGS to solve these VRPs.
