"""
Wrapper file for building the native extensions. This file wraps the
build_extensions script, and uses a temporary build directory for Meson.
That's useful for ``uv sync`` and the default build backend, which should
work in isolation.
"""

import pathlib
import tempfile

from build_extensions import build, clean


def main():
    cwd = pathlib.Path.cwd()

    with tempfile.TemporaryDirectory() as tmpdir:
        build_dir = pathlib.Path(tmpdir)
        install_dir = cwd / "pyvrp"

        clean(build_dir, install_dir)
        build(build_dir, build_type="release", verbose=False)


if __name__ == "__main__":
    main()
