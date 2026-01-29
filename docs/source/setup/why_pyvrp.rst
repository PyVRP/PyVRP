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

Each of the projects we consider her has its own strengths and weaknesses, depending on where the project's focus lies.
We do not aim for a completely exhaustive discussion, but instead try to focus on the following aspects: *scale*, *solution quality*, *project activity*, *ease of modification*, and *ease of use*.
The following table provides a subjective overview of the relative strenghts of each project.
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
   * - Ease of modification
     - ⭐
     - ⭐⭐
     - ⭐⭐
     - ⭐⭐
   * - Ease of use
     - ⭐⭐⭐
     - ⭐⭐⭐
     - ⭐⭐
     - ⭐


Scale
^^^^^

We define *scale* as the size of the largest instances for which a reasonable solution can typically still be found.

TODO


Solution quality
^^^^^^^^^^^^^^^^

We define *solution quality* by how good the returned solution typically is, after a modest amount of runtime (at least a few minutes).

TODO


Project activity
^^^^^^^^^^^^^^^^

We define *activity* loosely by the project's overall community size, as evidenced by the activity on its issue tracker and development activity.

TODO


Ease of modification
^^^^^^^^^^^^^^^^^^^^

We define *ease of modification* by how easy it is to make changes to the project's codebase, particularly to adjust or implement your own features.

TODO


Ease of use
^^^^^^^^^^^

We define *ease of use* by how easy it is to define and solve a vehicle routing problem using the software.

TODO
