import datetime
import glob
import os
import shutil
import sys
from pathlib import Path

import tomli

# -- Project information

sys.path.insert(0, os.path.abspath("../../"))

now = datetime.date.today()

project = "PyVRP"
author = "PyVRP contributors"
copyright = f"2019 - {now.year}, {author}"

with open("../../pyproject.toml", "rb") as fh:
    pyproj = tomli.load(fh)
    release = version = pyproj["tool"]["poetry"]["version"]

for file in glob.iglob("../../examples/*.ipynb"):
    path = Path(file)

    print(f"Copy {path.name} into docs/source/examples/")
    shutil.copy2(path, f"examples/{path.name}")

# -- Autodoc

autoclass_content = "class"

autodoc_member_order = "bysource"

autodoc_default_flags = ["members"]

# -- Numpydoc

numpydoc_xref_param_type = True
numpydoc_class_members_toctree = False

# -- nbsphinx

nbsphinx_execute = "never"

# -- General configuration

extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    # "sphinx.ext.autosummary",
    "sphinx.ext.intersphinx",
    "sphinx_rtd_theme",
    "nbsphinx",
    "numpydoc",
]

exclude_patterns = ["_build", "**.ipynb_checkpoints"]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "sphinx": ("https://www.sphinx-doc.org/en/master/", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "matplotlib": ("https://matplotlib.org/stable/", None),
}
intersphinx_disabled_domains = ["std"]

templates_path = ["_templates"]

# -- Options for HTML output

html_theme = "sphinx_rtd_theme"

# -- Options for EPUB output
epub_show_urls = "footnote"
