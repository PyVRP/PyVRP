import datetime
import importlib
import inspect
import os
import shutil
import subprocess
import sys
from typing import Optional

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


# -- sphinx.ext.linkcode
REVISION_CMD = "git rev-parse --short HEAD"


def _get_git_revision() -> str:
    """
    Returns the current git revision as a string. If the revision cannot be
    determined, returns "main".
    """
    try:
        revision = subprocess.check_output(REVISION_CMD.split()).strip()
    except (subprocess.CalledProcessError, OSError):
        print("Failed to execute git to get revision: returning 'main'.")
        return "main"
    return revision.decode("utf-8")


def linkcode_resolve(domain: str, info: dict) -> Optional[str]:
    """
    Returns the URL to the GitHub source code corresponding to a object.

    Parameters
    ----------
    domain: str
        The domain of the object (e.g., "py" for Python, "cpp" for C++).
    info: dict
        Dictionary containing the module and fullname of the object.

    Returns
    -------
    Optional[str]
        The URL to the GitHub source code corresponding to the object.
        If a URL cannot be generated, returns None instead.
    """

    if domain != "py" or not info.get("module") or not info.get("fullname"):
        return None  # only allow Python objects with complete module and name.

    # Find the object.
    module = importlib.import_module(info["module"])

    if "." in info["fullname"]:  # object is a method or attribute of a class
        obj_name, attr_name = info["fullname"].split(".")
        obj = getattr(module, obj_name)

        try:
            obj = getattr(obj, attr_name)  # object is a method of a class
        except AttributeError:
            return None  # object is an attribute of a class
    else:
        obj = getattr(module, info["fullname"])

    # Find the path to the source file and the object's start line number.
    try:
        source_file = inspect.getsourcefile(obj)
        start_line_no = inspect.getsourcelines(obj)[1]  # first line number

        assert source_file is not None
        rel_path = os.path.relpath(source_file, os.path.abspath(".."))
    except Exception:
        return None

    base_url = "https:///github.com/PyVRP/PyVRP/blob"
    revision = _get_git_revision()
    return f"{base_url}/{revision}/{rel_path}#L{start_line_no}"


# -- numpydoc
numpydoc_xref_param_type = True
numpydoc_class_members_toctree = False
numpydoc_attributes_as_param_list = False
napoleon_include_special_with_doc = True

# -- nbsphinx
nbsphinx_execute = "always"

# -- General configuration
extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.linkcode",
    "sphinx.ext.napoleon",
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
python_use_unqualified_type_names = True

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
            "version": "https://pyvrp.github.io/v0.5.0",
            "title": "v0.5.0",
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

python_resolve_unqualified_typing = True
python_transform_type_annotations_pep585 = True
python_transform_type_annotations_pep604 = True
object_description_options = [
    ("py:.*", dict(include_fields_in_toc=False, include_rubrics_in_toc=False)),
    ("py:attribute", dict(include_in_toc=False)),
    ("py:parameter", dict(include_in_toc=False)),
]


# -- Options for EPUB output
epub_show_urls = "footnote"
