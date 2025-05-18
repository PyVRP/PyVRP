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

Now, change into the PyVRP directory, and set-up the virtual environment using ``uv``:

.. code-block:: shell

   cd PyVRP
   uv sync

Syncing might take a little while, but only needs to be done once.
Now make sure everything runs smoothly, by executing the test suite:

.. code-block:: shell

   uv run pytest

.. note::

   If you use `pre-commit <https://pre-commit.com/>`_, you can use our pre-commit configuration file to set that up too.
   Simply type:

   .. code-block:: shell

      pre-commit install

   After this completes, style and typing issues are automatically checked whenever you make a new commit to your feature branch.


Setting up Github Codespaces
----------------------------

If you are having trouble building PyVRP from source or setting up your local development environment, you can try to build PyVRP online, using `GitHub Codespaces <https://docs.github.com/en/codespaces>`_.
Github Codespaces allows you to create a development environment directly in your browser.

To launch Github Codespaces, go to the `PyVRP repository <https://github.com/PyVRP/PyVRP>`_ and click on the green button.
Select the Codespaces tab and click on the `+` icon to create a Codespaces environment.
This environment is configured with all necessary dependencies to build PyVRP.
Once the setup completes, execute the test suite to verify everything runs smoothly:

.. code-block:: shell

   uv run pytest


Building the extensions
-----------------------

PyVRP uses a number of native Python extensions that are written in C++ for performance.
These extensions are built every time ``uv sync`` is used, but that command builds everything in release mode.
While developing, one typically wants to use debug builds.
Now that the virtual environment has been prepared, these (and more) can be made by using the ``buildtools/build_extensions.py`` script directly, as follows:

.. code-block:: shell

   uv run buildtools/build_extensions.py

The script takes a number of command-line arguments, which you can discover using

.. code-block:: shell

   uv run buildtools/build_extensions.py --help

We use the Meson build system to compile the C++ extensions.
Meson is configured using the ``meson.build`` file in the repository root. 
You should not have to touch this file often: all compilation is handled via the ``buildtools/build_extensions.py`` script.


Debugging the extensions
------------------------

This section explains how to perform cross-debugging for mixed Python and C++ code.
We will use the `Visual Studio Code <https://code.visualstudio.com/>`_ IDE and the `Python C++ Debug <https://github.com/benibenj/vscode-pythonCpp>`_ extension.

First, build PyVRP in debug mode:

.. code-block:: shell

   uv run buildtools/build_extensions.py --build_type debug

Create a test Python file that calls some C++ code, like so:

.. code-block:: python

   from pyvrp import Client

   Client(x=0, y=0)

Set breakpoints in ``pyvrp/cpp/ProblemData.cpp`` within the ``Client`` constructor.
Next, set-up your debugger configuration by creating the ``.vscode/launch.json`` file, with the following content:

.. code-block:: json

   {
       "version": "0.2.0",
       "configurations": [
           {
               "name": "Python C++ Debugger",
               "type": "pythoncpp",
               "request": "launch",
               "pythonConfig": "default",
               "cppConfig": "default (gdb) Attach"
           }
       ]
   }

Start the debugger in Visual Studio Code and step through the code.
The debugger should break at the breakpoints that you set in ``pvvrp/cpp/ProblemData.cpp``.


Profiling the extensions
------------------------

Typically, the most computationally intense components in PyVRP are written in C++, as native extensions.
While developing new functionality that touches the C++ components, it is important to pay attention to performance.
For this, profiling is an incredibly useful tool.
There are many ways to get started with profiling, but the following may be helpful.

First, build a debug optimised build of PyVRP, as follows:

.. code-block:: shell

   uv run buildtools/build_extensions.py --build_type debugoptimized

This ensures all debug symbols are retained, so the profiling output contains meaningful information.
Next, we need to use a profiling tool, which varies based on your operating system.

.. md-tab-set::

    .. md-tab-item:: Linux

        Make sure you install ``perf``, the Linux profiling tool.
        Now, all we need to do is let ``perf`` record PyVRP doing some work, like for example:

        .. code-block:: shell

            uv run perf record pyvrp instances/VRPTW/RC2_10_5.vrp --seed 6 --round_func dimacs --max_runtime 5

        The resulting ``perf.data`` file will contain all relevant profiling results.
        Such a file can be inspected using ``perf`` on the command line, or with a GUI using, for example, KDAB's `hotspot <https://github.com/KDAB/hotspot>`_ program.

    .. md-tab-item:: macOS

        macOS comes with a profiling tool named Instruments, which is bundled inside Apple's `Xcode <https://developer.apple.com/xcode/>`_.
        First, make sure you have Xcode installed.
        Now, run PyVRP for a period of time long enough that we can attach to the corresponding process, like for example:

        .. code-block:: shell

            uv run pyvrp instances/VRPTW/RC2_10_5.vrp --seed 6 --round_func dimacs --max_runtime 60

        Next, open the Instruments application.
        Select the "CPU Profiler" template, click on the search bar at the top of the window, and select the corresponding Python process as your target, which is usually the most recent one.
        Start profiling by clicking on the red circle in the top-left corner.
        Once you are ready, you can stop the profiling and analyze the results.

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


Licensing
---------

PyVRP is licensed under the MIT license.
All code, documentation and other files added to PyVRP by contributors is licensed under this license, unless another license is explicitly specified in the source file.
For your contribution, please check that it can be included into PyVRP under the MIT license.
If you did not write the code yourself, you must ensure that the existing license is compatible and include the license information in the contributed files, or obtain permission from the original author to relicense the contributed code.
Contributors keep the copyright for code they wrote and submit for inclusion to PyVRP.
