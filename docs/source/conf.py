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
authors = "PyVRP contributors"
copyright = f"2022 - {now.year}, {authors}"

with open("../../pyproject.toml", "rb") as fh:
    pyproj = tomli.load(fh)
    release = version = pyproj["tool"]["poetry"]["version"]

for file in glob.iglob("../../examples/*.ipynb"):
    path = Path(file)

    print(f"Copy {path.name} into docs/source/examples/")
    shutil.copy2(path, f"examples/{path.name}")

# -- Autoapi and autodoc

autoapi_type = "python"
autoapi_dirs = ["../../pyvrp"]
autoapi_options = ["members", "undoc-members", "special-members"]
autoapi_ignore = ["*/tests/*.py", "*/cli.py"]

autoapi_generate_api_docs = False
autoapi_add_toctree_entry = False
autoapi_add_objects_to_toctree = False

autoapi_python_class_content = "class"
autoapi_member_order = "bysource"

autodoc_typehints = "signature"

# -- Numpydoc

numpydoc_xref_param_type = True
numpydoc_class_members_toctree = False
napoleon_include_special_with_doc = True

# -- nbsphinx

nbsphinx_execute = "never"

# -- General configuration

extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "autoapi.extension",
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

add_module_names = False

# -- Options for HTML output

html_theme = "sphinx_rtd_theme"

# -- Options for EPUB output
epub_show_urls = "footnote"
