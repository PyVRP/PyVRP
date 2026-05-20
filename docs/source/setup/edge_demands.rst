Modelling Edge Demands
======================

PyVRP supports optional directed, profile-specific edge demand matrices.
Use these when resource consumption depends on the traversed arc :math:`i \to j`,
instead of only on the visited client.


When to use edge demands
------------------------

Edge demands are useful for route-dependent resource consumption, for example:

* energy/fuel usage that depends on the exact transition between locations;
* toll/permit usage modelled as a consumable capacity budget;
* transition-dependent handling effort that should count against a limited
  vehicle resource.

If your resource use is purely client-based, client pickup/delivery demand is
typically enough and simpler.


Semantics
---------

For each route and load dimension:

.. math::

   \text{used load}
   = \sum \text{client demand}
   + \sum \text{edge demand}(i, j)

Route feasibility checks this used load against the vehicle capacity, just like
regular load constraints.

Edge demands are:

* directed (``i -> j`` may differ from ``j -> i``);
* profile-specific, just like distances and durations;
* optional (if omitted, they are treated as zero everywhere);
* currently non-negative.


API usage
---------

Use :meth:`~pyvrp.Model.Model.add_edge` with ``edge_demands``:

.. code-block:: python

   model.add_edge(
       frm=loc_i,
       to=loc_j,
       distance=1200,
       duration=180,
       profile=truck_profile,        # optional
       edge_demands=[4, 1],          # one value per load dimension
   )

``profile=None`` (default) creates a base edge that is inherited by all
profiles, unless a profile-specific edge overrides it.


Example: directed and profile-specific
--------------------------------------

.. code-block:: python

   base = model.add_profile(name="base")
   ev = model.add_profile(name="ev")

   # Base edges (apply to all profiles unless overridden).
   model.add_edge(a, c, distance=10, duration=10, edge_demands=[5])
   model.add_edge(b, c, distance=10, duration=10, edge_demands=[20])

   # Override one arc for EV profile only.
   model.add_edge(a, c, distance=10, duration=10, profile=ev, edge_demands=[2])

This allows, for example, ``A -> C`` and ``B -> C`` to consume different
amounts, and lets consumption differ per routing profile.


Caveats and validation rules
----------------------------

* Self-loop edge demands must be zero.
* Edge demand entries must be non-negative.
* Matrix shape must match the number of locations.
* The number of edge-demand dimensions must match the number of load
  dimensions.
* When edge demands are not provided, the model behaves as before (no edge
  demand contribution).


Reload semantics
----------------

Edge demands are treated as monotone route-level consumption.
They are **not reset** by reload depots.

Reload depots reset pickup/delivery load semantics, but edge-demand consumption
continues accumulating over the full route.

See also :doc:`faq` for concise modelling Q&A.
