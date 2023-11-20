The VRPLIB format
=================

PyVRP supports reading benchmark instances in the VRPLIB format through the `VRPLIB <https://github.com/leonlan/VRPLIB>`_ package.
Although generally used, this format is not all that well standardised, so it merits some description what PyVRP actually supports.
In addition to standard specifications and data sections, PyVRP uses several data sections that are not commonly found in (other) benchmark instances.
On this page, we explain all specifications and data sections that PyVRP understands.

This page can be useful in understanding how benchmark instances should be formatted to work with PyVRP's :meth:`~pyvrp.read.read` function.
All instances in our `instances repository <https://github.com/PyVRP/Instances>`_ adhere to this format, and use the specifications and data sections described below.

.. note::

   Any specification or data section in a VRPLIB-formatted file that is *not* defined in the glossaries below is not understood by PyVRP, and will be silently ignored.


Specifications
--------------

Specifications are key-value pairs.
PyVRP supports the following specifications:

.. glossary::
   :sorted:

   ``CAPACITY``
      Homogeneous capacity of all vehicles in the instance.
      The number of vehicles is provided using the :term:`VEHICLES` specification.

   ``DIMENSION``
      Number of locations in the instance.
      This is the sum of the number of depots and the number of clients.

   ``EDGE_WEIGHT_FORMAT``
      Specifies the format of the :term:`EDGE_WEIGHT_SECTION` if the edge weights are given explicitly.

   ``EDGE_WEIGHT_TYPE``
      Specifies how the edge weights (distances and durations) are given.

   ``VEHICLES``
      Number of vehicles in the instance.
      The number of vehicles defaults to the number of clients in the instance when this specification is not provided: PyVRP assumes an unlimited fleet in this case.


Data sections
-------------

Data sections are array-like values that specify the actual problem data.
PyVRP supports the following data sections:

.. glossary::
   :sorted:

   ``DEMAND_SECTION``
      Array of demands, one for each location.
      The depot locations *must* have zero demand.

   ``DEPOT_SECTION``
      Array of location indices that are depots.

   ``EDGE_WEIGHT_SECTION``
      When provided, this section explicitly describes the distance and duration matrices.
      If not provided, such matrices are computed based on what's specified for :term:`EDGE_WEIGHT_FORMAT` and :term:`EDGE_WEIGHT_TYPE`.

   ``NODE_COORD_SECTION``
      Array of :math:`(x, y)` coordinates for each location.

   ``PRIZE_SECTION``
      Array of prizes for visiting each location.
      Must be zero for depots.
      A value of zero for non-depots implies visiting that location is required.

   ``RELEASE_TIME_SECTION``
      Array of release times for each location.
      Must be zero for depots.

   ``SERVICE_TIME_SECTION``
      Array of service durations for each location.
      Must be zero for depots.

   ``TIME_WINDOW_SECTION``
      Array of :math:`[e, l]` time window data, for each location.
