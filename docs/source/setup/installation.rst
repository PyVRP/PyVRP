Installation instructions
=========================

The most straightforward way to use the ``pyvrp`` package in your project is to install it directly from the Python package index, like so:

.. code-block:: shell

   pip install pyvrp

Alternatively, PyVRP can be installed from directly from the source code on GitHub:

.. code-block:: shell

   pip install 'pyvrp @ git+https://github.com/PyVRP/PyVRP'

This can be useful to get updates that have not yet made it to the Python package index.

Running the examples locally
----------------------------

To run the example notebooks locally, first clone the repository:

.. code-block:: shell

   git clone https://github.com/PyVRP/PyVRP.git

Then, make sure to have `uv <https://docs.astral.sh/uv/getting-started/installation/>`_ installed.
One way to do so is via ``pip``:

.. code-block:: shell

   pip install --upgrade uv

Now, go into the PyVRP repository and set-up a virtual environment.
We want a virtual environment that also contains all dependencies needed to run the example notebooks, so we also need to install the optional ``examples`` dependency group.
This goes like so:

.. code-block:: shell

   cd PyVRP
   uv sync --group examples

This might take a minute to resolve, but only needs to be done once.
After setting everything up, simply open the jupyter notebooks:

.. code-block:: shell

   uv run jupyter notebook

This will open up a page in your browser, where you can navigate to the example notebooks in the ``examples/`` folder!
