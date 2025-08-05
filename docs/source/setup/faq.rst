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

   How do I model vehicle load or service duration at the depots?

      PyVRP's :class:`~pyvrp._pyvrp.Depot` object indeed does not have a load or service duration attribute.
      Instead, this can be modelled by adding the time for loading or servicing the vehicle at the depot to the duration of all the edges leaving the depot.
      This is an equivalent way of modelling depot service duration.

   How can I model vehicle-specific service durations?

      Rather than specifying an explicit service duration for the clients or depots, add the service duration to the duration all the edges leaving the location.
      Using route profiles, you can then set up different travel duration matrices for the vehicles that include the vehicle-specific service durations.

Debugging
---------

.. glossary::

   PyVRP seems to get stuck with partial travel matrices. How do I prevent this from happening?

      PyVRP internally uses a large ``missing_value`` constant for the distance and duration on missing edges.
      This sometimes results in integer overflow, which causes the solver to get stuck.
      If this is happening to you, please pass an explicit ``missing_value`` argument to :meth:`~pyvrp.Model.Model.solve` that is more in line with your particular data instance.
