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
