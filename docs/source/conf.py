import datetime
import os
import shutil
import sys

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

print("Copying example notebooks into docs/source/examples/")
shutil.copytree("../../examples", "examples/", dirs_exist_ok=True)

# -- API documentation
autoclass_content = "class"
autodoc_member_order = "bysource"
autodoc_typehints = "signature"

# -- numpydoc
numpydoc_xref_param_type = True
numpydoc_class_members_toctree = False
napoleon_include_special_with_doc = True

# -- nbsphinx
nbsphinx_execute = "always"

# -- General configuration
extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx_immaterial",
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
html_theme = "sphinx_immaterial"
html_logo = "assets/images/icon.svg"
html_theme_options = {
    "site_url": "https://pyvrp.org/",
    "repo_url": "https://github.com/PyVRP/PyVRP/",
    "icon": {
        "repo": "fontawesome/brands/github",
        "edit": "material/file-edit-outline",
    },
    "features": [
        "navigation.expand",
        "navigation.top",
    ],
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "primary": "orange",
            "accent": "yellow",
            "scheme": "default",
            "toggle": {
                "icon": "material/lightbulb-outline",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "primary": "orange",
            "accent": "yellow",
            "scheme": "slate",
            "toggle": {
                "icon": "material/lightbulb",
                "name": "Switch to light mode",
            },
        },
    ],
    "version_dropdown": True,
    "version_info": [
        {
            "version": "",
            "title": "Development",
            "aliases": [],
        },
        {
            "version": "https://pyvrp.github.io/v0.4.4",
            "title": "v0.4.4",
            "aliases": [],
        },
        {
            "version": "https://pyvrp.github.io/v0.3.0",
            "title": "v0.3.0",
            "aliases": [],
        },
        {
            "version": "https://pyvrp.github.io/v0.2.1",
            "title": "v0.2.1",
            "aliases": [],
        },
        {
            "version": "https://pyvrp.github.io/v0.1.0",
            "title": "v0.1.0",
            "aliases": [],
        },
    ],
}

# -- Options for EPUB output
epub_show_urls = "footnote"
