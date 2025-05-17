import pathlib
import tempfile

from build_extensions import build, clean


def main():
    cwd = pathlib.Path.cwd()

    with tempfile.TemporaryDirectory() as tmpdir:
        build_dir = pathlib.Path(tmpdir)
        install_dir = cwd / "pyvrp"

        clean(build_dir, install_dir)
        build(build_dir, "release", "vrptw", verbose=False)


if __name__ == "__main__":
    main()
