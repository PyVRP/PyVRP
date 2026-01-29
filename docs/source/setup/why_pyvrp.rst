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
We do not aim for a completely exhaustive discussion, but instead try to focus on the following aspects: *scale*, *solution quality*, *activity*, *ease of modification*, and *ease of use*.
We define these as follows:

* *Scale*: size of the largest instance for which a reasonable solution can typically still be found.
* *Solution quality*: how good the returned solution typically is.
* *Activity*: overall project community size and development activity.
* *Ease of modification*: how easy it to adjust the existing code implementation.
* *Ease of use*: how easy it is to define and solve a vehicle routing problem using the softwares.

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
   * - Activity
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


PyVRP
^^^^^

TODO


VROOM
^^^^^

TODO


jsprit
^^^^^^

TODO


OR-Tools
^^^^^^^^

TODO
