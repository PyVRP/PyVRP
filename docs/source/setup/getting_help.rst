Getting help
============

All conversations take place in the `GitHub repository <https://github.com/PyVRP/PyVRP/>`_.
If you are looking for help using PyVRP, please browse our :doc:`FAQ <faq>` and `discussions <https://github.com/PyVRP/PyVRP/discussions>`_ overview first for relevant discussions, questions and answers, modelling tricks, and more.
Feel free to open your own discussion thread if you have something new to discuss!


Submitting a bug report
-----------------------

Open a new issue in the repository, using the "Bug report" template.
To limit the amount of time needed to triage your problem, please do the following:

- Include a short, self-contained code snippet that reproduces the problem.
  If the problem relates to a particular VRP instance, include how to create it in your snippet - do not provide just raw data.
- Specify the version information of the ``pyvrp`` installation you use.
  You can do this by including the output of :meth:`~pyvrp.show_versions.show_versions`:

  .. code-block:: python

     import pyvrp
     pyvrp.show_versions()

  in your report.
  You can run this from the command line as

  .. code-block:: shell

     python -c 'import pyvrp; pyvrp.show_versions()'


Submitting a feature request
----------------------------

Please first browse the existing issues and discussions in the GitHub repository to see if your feature has already been requested.
If it has not, please open a new issue in the repository, using the "Feature request" template.


Professional support
--------------------

PyVRP is maintained by `RoutingLab <https://routinglab.tech>`_, a startup focused on next-generation route optimisation software. RoutingLab provides the following services:

- **Implementation support:** Get help with problem modeling, solver tuning, or implementation review for your production systems.
- **Custom features:** We develop features that align with our roadmap or are funded by you. Browse  `the list of fundable features <https://github.com/PyVRP/PyVRP/issues?q=sort%3Aupdated-desc%20is%3Aissue%20is%3Aopen%20label%3A%22waiting%20for%20funding%22>`_ or open your own feature request.
- **FastVRP API:** For a production-ready route optimisation API without implementation overhead, try out `FastVRP <https://routinglab.tech#fastvrp>`_.

Reach us at info@routinglab.tech
