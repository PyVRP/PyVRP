.. module:: pyvrp
   :synopsis: PyVRP


PyVRP
=====

The top-level :mod:`pyvrp` module exposes several core classes needed to run the VRP solver.
These include the core :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`, and the :class:`~pyvrp.Population.Population` that manages :class:`~pyvrp._Individual.Individual` solutions.
Most classes take parameter objects that allow for advanced configuration - but sensible defaults are also provided.
Finally, after running, the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm` returns a :class:`~pyvrp.Result.Result` object.
This object can be used to obtain the best observed solution, and detailed runtime statistics.

.. hint::

   Have a look at the examples to see how these classes relate!

.. automodule:: pyvrp.GeneticAlgorithm

   .. autoapiclass:: GeneticAlgorithmParams
      :members:

   .. autoapiclass:: GeneticAlgorithm
      :members:

.. automodule:: pyvrp._Individual
   
   .. autoapiclass:: Individual
      :members:
      :special-members:

.. automodule:: pyvrp._PenaltyManager

   .. autoapiclass:: PenaltyParams
      :members:

   .. autoapiclass:: PenaltyManager
      :members: 

.. automodule:: pyvrp.Population

   .. autoapiclass:: PopulationParams
      :members:

   .. autoclass:: Population
      :members:  
      :special-members: __iter__, __len__
 
.. automodule:: pyvrp._ProblemData

   .. autoapiclass:: Client
      :members:

   .. autoapiclass:: ProblemData
      :members:

.. automodule:: pyvrp.read

   .. autofunction:: read

   .. autofunction:: read_solution

.. automodule:: pyvrp.Result
   :members:

.. automodule:: pyvrp.show_versions
   :members:

.. automodule:: pyvrp.Statistics
   :members:

.. automodule:: pyvrp._XorShift128

   .. autoapiclass:: XorShift128
      :members:
      :special-members:

.. TODO add pyvrp CLI, and think about missing files (Matrix, TWS?)
