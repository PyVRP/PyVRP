import argparse
import os
import pathlib
from subprocess import check_call
from typing import List


def parse_args():
    parser = argparse.ArgumentParser(prog="build_extensions")

    parser.add_argument(
        "--build_dir",
        default="build",
        help="Directory for Meson to use while building extensions.",
    )
    parser.add_argument(
        "--build_type",
        default="release",
        choices=["debug", "debugoptimized", "release"],
        help="The type of build to provide. Defaults to release mode.",
    )
    parser.add_argument(
        "--problem",
        default="vrptw",
        choices=["cvrp", "vrptw"],
        help="Which type of solver to compile. Defaults to 'vrptw'.",
    )
    parser.add_argument(
        "--precision",
        default="integer",
        choices=["integer", "double"],
        help="Double is more precise, integer faster. Defaults to 'integer'.",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build and installation directories before building.",
    )
    parser.add_argument(
        "--regenerate_type_stubs",
        action="store_true",
        help="""
        Whether to regenerate the MyPy type stubs as well. Default False, since
        this can overwrite manual adjustments to the type stubs.
        """,
    )
    parser.add_argument(
        "--additional",
        nargs=argparse.REMAINDER,
        default=[],
        help="Extra Meson configuration options (passed verbatim to Meson).",
    )

    return parser.parse_args()


def clean(build_dir: pathlib.Path, install_dir: pathlib.Path):
    check_call(["rm", "-rf", str(build_dir)])

    for extension in install_dir.rglob("*.so"):
        extension.unlink()

    for extension in install_dir.rglob("*.pyd"):
        extension.unlink()


def regenerate_stubs(install_dir: pathlib.Path):
    def regen(extension):
        ext_dir = extension.parent
        ext_name, _ = extension.name.split(".", maxsplit=1)

        check_call(
            ["stubgen", "--parse-only", "-o", ".", "-m", ext_name],
            cwd=ext_dir,
        )

    for extension in install_dir.rglob("*.so"):
        regen(extension)

    for extension in install_dir.rglob("*.pyd"):
        regen(extension)


def build(
    build_dir: pathlib.Path,
    build_type: str,
    problem: str,
    precision: str,
    additional: List[str],
):
    cwd = pathlib.Path.cwd()
    args = [
        # fmt: off
        "--buildtype", build_type,
        f"-Dpython.platlibdir={cwd.absolute()}",
        f"-Dproblem={problem}",
        f"-Dstrip={'true' if build_type == 'release' else 'false'}",
        f"-Dprecision={precision}",
        *additional,
        # fmt: on
    ]

    cmd = "configure" if build_dir.exists() else "setup"
    check_call(["meson", cmd, build_dir, *args])
    check_call(["meson", "compile", "-C", build_dir])
    check_call(["meson", "install", "-C", build_dir])


def main():
    args = parse_args()
    cwd = pathlib.Path.cwd()
    build_dir = cwd / args.build_dir
    install_dir = cwd / "pyvrp"

    if args.clean or os.environ.get("CIBUILDWHEEL", "0") == "1":
        # Always start from an entirely clean build when building wheels in
        # the CI. Else only do so when expressly asked.
        clean(build_dir, install_dir)

    build(
        build_dir,
        args.build_type,
        args.problem,
        args.precision,
        args.additional,
    )

    if args.regenerate_type_stubs:
        regenerate_stubs(install_dir)


if __name__ == "__main__":
    main()
