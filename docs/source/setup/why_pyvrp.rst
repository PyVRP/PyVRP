Why choose PyVRP?
=================

PyVRP is one of several open-source vehicle routing solver alternatives, like `VROOM <https://github.com/VROOM-Project/vroom/>`_, `jsprit <https://github.com/graphhopper/jsprit>`_, or implementing a solver yourself using the building blocks provided by `OR-Tools <https://github.com/google/or-tools>`_.
Why choose us for your route optimisation, rather than one of those alternatives?
In this document we try to give a structured answer to this question, by comparing features, and project-specific strengths and weaknesses.

Feature comparison
------------------

Our aim is to provide a neutral and complete overview of each solver's main features.

.. note::
   The following table does not include OR-Tools, by design: OR-Tools can be used to implement essentially any feature you'd like, but *you* need to do the implementation.
   This approach offers tremendous flexibility, at the cost of (significant) implementation work.

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
   * - Multiple trips (reloading)
     - ✅
     - ❌
     - ❌
   * - Shipments (pickup-and-delivery)
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
   * - Alternative visits
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
   * - Skills
     - ✅ (via load constraints)
     - ✅
     - ✅


Strengths and weaknesses
------------------------

Each of the projects we compare here has its own strengths and weaknesses.
We do not aim for a completely exhaustive discussion, but instead try to focus on the following aspects: *scale*, *solution quality*, *project activity*, *ease of use*, and *ease of modification*.
The following table provides a subjective overview of the relative strengths of each project.
We discuss these further below.

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
     - ⭐⭐
     - ⭐
   * - Project activity
     - ⭐⭐
     - ⭐⭐⭐
     - ⭐ 
     - ⭐⭐⭐
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
PyVRP combines an effective metaheuristic with a very efficient local search, and typically finds near-optimal solutions very quickly.
VROOM has a strong local search, but no metaheuristic on top: its solutions are good, but it can get stuck in a mediocre local optimum.
jsprit implements a ruin-and-recreate metaheuristic, but does not efficiently evaluate moves, which means it cannot quickly evaluate many moves.
Its performance is comparable to VROOM on medium-sized instances. 
OR-Tools implements a guided local search, which is not very effective in practice.


Project activity
^^^^^^^^^^^^^^^^

We define *project activity* loosely based on the project's issue tracker, discussion, and development activity.
PyVRP is under active development, but is still fairly new, and has a relatively small user and developer community.
Both VROOM and OR-Tools are large, established projects, with an active user and developer community around them.
Finally, jsprit appears largely abandoned, with few, sporadic commits and posts to their issue tracker.


Ease of use
^^^^^^^^^^^

We define *ease of use* by how easy it is to define and solve a vehicle routing problem using the software.
PyVRP is straightforward to use, with clear examples, documentation, and a high-level modelling interface (:class:`~pyvrp.Model.Model`).
VROOM similarly provides clear examples and API documentation.
jsprit provides documentation inside their repository, but those are nearly a decade old and only contain code snippets, not complete examples.

Finally, OR-Tools offers significant documentation, lots of examples, and an active discussion forum.
These are also needed because the user needs to implement all their routing constraints mostly from scratch, and that requires some understanding of OR-Tools' constraint programming model.
We regularly hear from our users that this is a significant burden, both for initial setup and continued maintance.


Ease of modification
^^^^^^^^^^^^^^^^^^^^

We define *ease of modification* by how easy it is to make changes to the project's codebase, particularly to adjust or implement your own features.
VROOM and jsprit are fully implemented in C++ and Java, respectively, and relatively straightforward to work with if you understand the language.
OR-Tools is a complicated C++ codebase, but one rarely needs to dive deep into its internals since OR-Tools already offers a great deal of flexibility for implementing custom requirements.

Finally, PyVRP implements both C++ and Python components.
Extending PyVRP thus requires a solid understanding of both languages.
Additionally, PyVRP uses various concepts from the operational research literature to speed up its local search component, most notably its :term:`concatenation schemes <Concatenation scheme>` and :term:`route segments <Route segment>`.
These concepts can certainly be learned, but some prior familiarity with the relevant literature is very helpful in getting up to speed quickly.
