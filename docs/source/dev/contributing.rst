Contributing
============

Conversations about development and issues take place in the `GitHub repository <https://github.com/PyVRP/PyVRP/>`_.
Feel free to open a new issue if you have something to discuss.


Setting up a local installation
-------------------------------

Make sure you have a reasonably modern C++ compiler.
Any recent version that supports (most of) the C++20 standard should do.
Once you have a compiler installed, you can proceed by forking the PyVRP repository from the `GitHub website <https://github.com/PyVRP/PyVRP/fork>`_.
Then, clone your new fork to some local environment:

.. code-block:: shell

   git clone https://github.com/<your username>/PyVRP.git

Now, change into the PyVRP directory, and set-up the virtual environment using ``poetry``:

.. code-block:: shell

   cd PyVRP

   pip install --upgrade poetry
   poetry install --with examples,docs,dev

This might take a few minutes, but only needs to be done once.
Now make sure everything runs smoothly, by executing the test suite:

.. code-block:: shell

   poetry run pytest

.. note::

   If you use `pre-commit <https://pre-commit.com/>`_, you can use our pre-commit configuration file to set that up too.
   Simply type:

   .. code-block:: shell

      pre-commit install

   After this completes, style and typing issues are automatically checked whenever you make a new commit to your feature branch.


Building the Python extensions
------------------------------

PyVRP uses a number of Python extensions that are written in C++ for performance.
These extensions are built every time ``poetry install`` is used, but that command builds everything in release mode.
While developing, one typically wants to use debug builds.
These (and more) can be made by using the ``build_extensions.py`` script directly, as follows:

.. code-block:: shell

   poetry run python build_extensions.py

The script takes a number of command-line arguments, which you can discover using

.. code-block:: shell

   poetry run python build_extensions.py --help

We use the Meson build system to compile the C++ extensions.
Meson is configured using the ``meson.build`` file in the repository root. 
You should not have to touch this file often: all compilation is handled via the ``build_extensions.py`` script.


Committing changes
------------------

We use pull requests to develop PyVRP.
For a pull request to be accepted, you must meet the below requirements.
This greatly reduces the job of maintaining and releasing the software.

- **One branch. One feature.**
  Branches are cheap and GitHub makes it easy to merge and delete branches with a few clicks.
  Avoid the temptation to lump in a bunch of unrelated changes when working on a feature, if possible.
  This helps us keep track of what has changed when preparing a release.
- Commit messages should be clear and concise.
  This means a subject line of less than 80 characters, and, if necessary, a blank line followed by a commit message body.
- Code submissions should always include tests.
- Each function, class, method, and attribute needs to be documented using docstrings.
  We conform to the `NumPy docstring standard <https://numpydoc.readthedocs.io/en/latest/format.html#docstring-standard>`_.
- If you are adding new functionality, you need to add it to the documentation by editing (or creating) the appropriate file in ``docs/source/``.
- Make sure your documentation changes parse correctly.
  See the documentation in the ``docs/`` directory for details on how to build the documentation locally.

.. note::

   Please use the "Pull request" template on GitHub when opening a pull request.
