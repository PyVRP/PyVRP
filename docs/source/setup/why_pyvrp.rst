Why choose PyVRP?
=================

There are several alternative open-source vehicle routing solvers, including `VROOM <https://github.com/VROOM-Project/vroom/>`_, `jsprit <https://github.com/graphhopper/jsprit>`_, or implementing a solver yourself using the building blocks provided by `OR-Tools <https://github.com/google/or-tools>`_.
Why choose PyVRP for your route optimisation, rather than one of those alternatives?
This page aims to help you decide through a structured comparison of features and project-specific strengths and weaknesses.


Feature comparison
------------------

.. note::
   The following table does not include OR-Tools, by design: OR-Tools can be used to implement essentially any feature you want, but *you* need to do the implementation.
   That approach offers tremendous flexibility, at the cost of (significant) implementation work.

.. list-table::
   :header-rows: 1

   * - Feature
     - PyVRP
     - VROOM
     - jsprit
   * - Load and capacity constraints
     - ✅
     - ✅
     - ✅
   * - Time windows
     - ✅ (multiple)
     - ✅ (multiple)
     - ✅ (multiple)
   * - Heterogeneous fleet
     - ✅
     - ✅
     - ✅
   * - Reloading [#reloading]_
     - ✅
     - ❌
     - ❌
   * - Shipments [#pickup_delivery]_
     - ❌
     - ✅
     - ✅
   * - Multiple depots
     - ✅
     - ✅
     - ✅
   * - Breaks
     - ❌
     - ✅ (fixed)
     - ✅ (fixed)
   * - Duration objective
     - ✅
     - ⚠️ (not wait duration)
     - ✅
   * - Alternative visits [#mutually_exclusive_groups]_
     - ✅
     - ❌
     - ❌
   * - Profiles
     - ✅
     - ✅
     - ✅
   * - Time-dependent travel durations
     - ❌
     - ❌
     - ✅  
   * - Release times
     - ✅
     - ❌
     - ❌
   * - Optional visits
     - ✅
     - ✅
     - ✅
   * - Skills [#skills]_
     - ✅ (via load constraints)
     - ✅
     - ✅


.. [#reloading]
   Have a look at our :doc:`../notebooks/reloading` tutorial to get started using this feature.

.. [#pickup_delivery]
   Shipments require an amount of goods to be picked up at one location and then delivered to another.
   PyVRP only supports shipments from and to the depot, not between general client visits.

.. [#mutually_exclusive_groups]
   Have a look at our :doc:`../notebooks/mutually_exclusive_groups` tutorial to get started using this feature.

.. [#skills]
   Matching visits and vehicles based on skills that the visit requires, and the vehicle must have.
   PyVRP uses load constraints for this; see our :doc:`../notebooks/load` tutorial for an example.


Strengths and weaknesses
------------------------

Each of the projects we compare here has its own strengths and weaknesses.
We do not aim for a completely exhaustive discussion, but instead focus on the following aspects: *scale*, *solution quality*, *project activity*, *ease of use*, and *ease of modification*.
The following table provides a balanced overview of each project.
We discuss our reasoning further below.

.. list-table::
   :header-rows: 1

   * - Aspect
     - PyVRP
     - VROOM
     - jsprit
     - OR-Tools
   * - Scale
     - ⭐⭐⭐
     - ⭐⭐⭐
     - ⭐⭐
     - ⭐⭐
   * - Solution quality
     - ⭐⭐⭐
     - ⭐⭐
     - ⭐
     - ⭐
   * - Project activity
     - ⭐⭐
     - ⭐⭐⭐
     - ⭐
     - ⭐⭐
   * - Ease of use
     - ⭐⭐⭐
     - ⭐⭐⭐
     - ⭐⭐
     - ⭐
   * - Ease of modification
     - ⭐
     - ⭐⭐
     - ⭐⭐
     - ⭐


Scale
^^^^^

We define *scale* as the size of the largest instances for which a reasonable solution can typically still be found.
Both PyVRP and VROOM can comfortably solve instances with many thousands of visits.
On the other hand, jsprit and OR-Tools struggle on instances above a thousand visits, with solution quality already deteriorating from about 500 visits onwards.


Solution quality
^^^^^^^^^^^^^^^^

.. hint::
   Have a look at our :doc:`benchmarks <benchmarks>`! 

We define *solution quality* by how good the returned solution typically is, after a modest amount of runtime (a few minutes).
PyVRP combines an effective metaheuristic with a very efficient local search, and typically finds near-optimal solutions quickly.
VROOM has a local search component, but no metaheuristic on top: it is fast and returns good solutions, but it can occasionally get stuck in a mediocre local optimum.
jsprit implements a ruin-and-recreate metaheuristic but does not efficiently evaluate moves, which means it cannot quickly iterate through the search space.
It performs adequately on small and medium-sized instances but can be poor on large instances, or those with shipments (pickup-and-delivery). 
OR-Tools implements a guided local search that performs similarly to jsprit on most routing problems, but is better on instances with shipments.


Project activity
^^^^^^^^^^^^^^^^

We define *project activity* based on the project's issue tracker, discussion, and development activity.
PyVRP is under active development, but is still fairly new, and has a relatively small user and developer community.
Both VROOM and OR-Tools are large, established projects, with active user and developer communities, although OR-Tools' routing components have not seen much feature development in recent years.
Finally, jsprit appears largely abandoned, with few, sporadic `commits <https://github.com/graphhopper/jsprit/commits/master/>`_ and posts to their `issue tracker <https://github.com/graphhopper/jsprit/issues>`_.


Ease of use
^^^^^^^^^^^

We define *ease of use* by how easy it is to define and solve a vehicle routing problem using the software.
PyVRP is straightforward to use, with clear examples, documentation, and a high-level modelling interface via its :class:`~pyvrp.Model.Model`.
VROOM similarly provides clear examples and API documentation.
jsprit provides documentation in their repository, but those are nearly a decade old and only contain code snippets, not complete examples.

Finally, OR-Tools offers significant documentation, lots of examples, and an active discussion forum.
These are also needed because the user needs to implement all their routing constraints mostly from scratch, and that requires some understanding of OR-Tools' constraint programming model.
We regularly hear from our users that this is a significant burden, both for initial setup and continued maintance.


Ease of modification
^^^^^^^^^^^^^^^^^^^^

We define *ease of modification* by how easy it is to make changes to the project's codebase, particularly to adjust or implement your own features.
VROOM and jsprit are fully implemented in C++ and Java, respectively, and relatively straightforward to work with if you understand the language.
OR-Tools is a complicated C++ codebase, but one rarely needs to dive deep into its internals since OR-Tools already offers a great deal of flexibility for implementing custom requirements.

Finally, PyVRP implements both C++ and Python components.
Extending PyVRP requires a solid understanding of both languages.
Additionally, PyVRP uses various concepts from the operational research literature to speed up its local search component, most notably in its :term:`concatenation schemes <Concatenation scheme>` and :term:`route segments <Route segment>`.
Some prior familiarity with the relevant literature is very helpful in getting up to speed quickly.
