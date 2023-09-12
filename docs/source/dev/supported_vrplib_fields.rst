Supported VRPLIB fields
=======================

PyVRP supports reading VRPLIB-formatted files through the `VRPLIB <https://github.com/leonlan/VRPLIB>`_ package.
Although generally used, this format is not all that well standardised, so it merits some description which fields PyVRP supports.
In addition to standard fields, PyVRP uses several fields that are not commonly found in (other) benchmark instances.
On this page, we explain the fields and data sections that PyVRP understands.

This can be useful in understanding how benchmark instances should be formatted to work with PyVRP.
All instances in our `instances repository <https://github.com/PyVRP/Instances>`_ adhere to this format, and use the fields and data sections described below.

.. note::

   Any section in a VRPLIB-formatted file that is *not* in the glossary below is not understood by PyVRP, and will be silently ignored.


.. glossary::

   ``CAPACITY``
      TODO

   ``DEMAND_SECTION``
      TODO
   
   ``DEPOT_SECTION``
      TODO

   ``DIMENSION``
      TODO

   ``EDGE_WEIGHT_FORMAT``
      TODO

   ``EDGE_WEIGHT_SECTION``
      TODO

   ``EDGE_WEIGHT_TYPE``
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

   ``VEHICLES``
      TODO
