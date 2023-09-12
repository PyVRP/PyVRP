The VRPLIB format
=================

PyVRP supports reading benchmark instances in the VRPLIB format through the `VRPLIB <https://github.com/leonlan/VRPLIB>`_ package.
Although generally used, this format is not all that well standardised, so it merits some description what PyVRP actually supports.
In addition to standard specifications and data sections, PyVRP uses several data sections that are not commonly found in (other) benchmark instances.
On this page, we explain all specifications and data sections that PyVRP understands.

This page can be useful in understanding how benchmark instances should be formatted to work with PyVRP's :meth:`~pyvrp.read.read` function.
All instances in our `instances repository <https://github.com/PyVRP/Instances>`_ adhere to this format, and use the specifications and data sections described below.

.. note::

   Any specification or data section in a VRPLIB-formatted file that is *not* in the glossary below is not understood by PyVRP, and will be silently ignored.


Specifications
--------------

Specifications are key-value pairs.
PyVRP supports the following specifications:

.. glossary::
   :sorted:

   ``CAPACITY``
      TODO

   ``DIMENSION``
      TODO

   ``EDGE_WEIGHT_FORMAT``
      TODO

   ``EDGE_WEIGHT_TYPE``
      TODO

   ``VEHICLES``
      TODO


Data sections
-------------

Data sections are array-like values that specify the actual problem data.
PyVRP supports the following data sections:

.. glossary::
   :sorted:

   ``DEMAND_SECTION``
      TODO
   
   ``DEPOT_SECTION``
      TODO

   ``EDGE_WEIGHT_SECTION``
      TODO

   ``NODE_COORD_SECTION``
      TODO

   ``PRIZE_SECTION``
      TODO

   ``RELEASE_TIME_SECTION``
      TODO

   ``SERVICE_TIME_SECTION``
      TODO

   ``TIME_WINDOW_SECTION``
      TODO
