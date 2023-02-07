# Documentation

This directory hosts the documentation. 
We use Sphinx for this.
The documentation has a few unique dependencies that are listed in the `requirements.txt` file.
These cannot be placed in the top-level `pyproject.toml` file, because we also use readthedocs to render our documentation, and they do not work nicely with poetry.

The Makefile in this directory can be used to build the documentation.
Running the command `make help` from this directory provides an overview of the available options.
In particular `make html` is useful, as that will build the documentation in the exact same way as it will be displayed on readthedocs later.

Finally, all Sphinx-related settings are configured in `docs/source/conf.py`.
