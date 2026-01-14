FAQ
===

This is a list of frequently asked questions about PyVRP.
Feel free to suggest new entries!

Installation
------------

.. glossary::

   How do I install the latest PyVRP from ``main`` without having to compile things myself?

      The latest prebuilt binaries can be found `here <https://github.com/PyVRP/PyVRP/actions/workflows/CD.yml>`_.

   How do I compile PyVRP from source on Windows machines?

      None of the PyVRP developers have a functioning Windows development enviroment available, so we cannot help you troubleshoot this.
      If you have difficulty figuring out how to build PyVRP on Windows, consider using the `Windows Subsystem for Linux (WSL) <https://learn.microsoft.com/en-us/windows/wsl/>`_ instead.

Modelling
---------

.. glossary::

   How can I model vehicle-specific service durations?

      Rather than specifying an explicit service duration for the clients or depots, add the service duration to the duration all the edges leaving the location.
      Using route profiles, you can then set up different travel duration matrices for the vehicles that include the vehicle-specific service durations.

Debugging
---------

.. glossary::

   Why does PyVRP sometimes get stuck when using partial travel matrices?

      PyVRP internally uses a large ``missing_value`` argument to set the distance and duration attributes of missing edges.
      This sometimes results in integer overflow, which can cause the solver to get stuck.
      Passing a smaller ``missing_value`` to :meth:`~pyvrp.Model.Model.solve` should fix this problem.
