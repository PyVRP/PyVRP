A brief introduction to ILS
===========================

PyVRP provides a high-performance implementation of the iterated local search (ILS) algorithm for vehicle routing problems (VRPs).
ILS is a single-trajectory algorithm that iteratively improves a solution through a cycle of perturbation, improvement and acceptance steps.
This simple yet effective approach balances between exploration and exploitation of the search space, leading to high-quality solutions.

.. note::

   For a more thorough introduction to ILS for VRPs, we refer to the works of `Lourenço et al. (2018) <https://link.springer.com/chapter/10.1007/978-3-319-91086-4_5>`_ and `Accorsi and Vigo (2021) <https://pubsonline.informs.org/doi/abs/10.1287/trsc.2021.1059>`_.


The ILS algorithm works as follows.

The algorithm begins with an initial solution which becomes the current and best solution.
In each iteration, the algorithm applies a perturbation operator to the current solution -- enough to escape local optima, but preserving useful structure.
This perturbed solution serves as a starting point for a local search procedure that attempts to find an improved solution.
An acceptance criterion then determines whether to adopt the new candidate solution as the current solution for the next iteration.
This criterion typically accepts improvements and occasionally accepts non-improving solutions to escape local optima.
If the candidate solution improves over the best solution found so far, it is also registered as the new best solution.
The algorithm continues until a provided stopping criterion is met, at which point it returns the best solution found.

In pseudocode, ILS works as follows:


| **Input:** initial solution :math:`s_0`
| **Output:** the best-found solution :math:`s^*`
| Set initial solution as best solution :math:`s^* \leftarrow s_0` and current solution :math:`s^\text{curr} \leftarrow s_0`
| **repeat** until stopping criterion is met:
|     Apply perturbation operator to generate :math:`s^\text{pert} = \text{Perturbation}(s^\text{curr})`
|     Apply local search to obtain :math:`s^\text{cand} = \text{LocalSearch}(s^\text{pert})`
|     **if** :math:`\text{AcceptanceCriterion}(s^\text{curr}, s^\text{cand})`:
|         :math:`s^\text{curr} \leftarrow s^\text{cand}`
|     **if** :math:`s^\text{cand}` has a better objective value than :math:`s^*`:
|         :math:`s^* \leftarrow s^\text{cand}`
| **return** :math:`s^*`

PyVRP provides the ILS algorithm, stopping criteria, acceptance criteria and various perturbation and local search procedures.

.. hint::

   See the :doc:`tutorial <../examples/quick_tutorial>` page to get started with PyVRP.
