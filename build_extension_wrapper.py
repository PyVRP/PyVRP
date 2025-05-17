import pathlib
import tempfile

from build_extensions import build


def main():
    with tempfile.TemporaryDirectory() as tmpdir:
        build_dir = pathlib.Path(tmpdir) / "build"
        build(build_dir, "release", "vrptw", False)


if __name__ == "__main__":
    main()
