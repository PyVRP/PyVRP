Why choose PyVRP?
=================

PyVRP is just one of many open-source vehicle routing solver alternatives, like `VROOM <https://github.com/VROOM-Project/vroom/>`_ or `jsprit <https://github.com/graphhopper/jsprit>`_.
Why choose us for your route optimisation, rather than one of those alternatives?
In this document we try to give a structured answer to this question, by comparing features, and project-specific strengths and weaknesses.

TODO mention OR-Tools, general optimisation packages

.. note::
   Our aim is to provide a factual and neutral overview of each project's features, strengths and weaknesses.
   Please contact us if you think we made a mistake!


Feature comparison
------------------

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
     - ✅ (many)
     - ✅ (many)
     - ✅ (many)
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
   * - Minimise duration
     - ✅
     - ⚠️ (only travel and service)
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

TODO along the following dims:

* scale (max instance size, solve speed)
* solution quality
* active development
* nuanced features
* ease of use
