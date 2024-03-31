import argparse
import os
import pathlib
from subprocess import check_call


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
        "--clean",
        action="store_true",
        help="Clean build and installation directories before building.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Whether to print more verbose compilation output.",
    )
    parser.add_argument(
        "--use_pgo",
        action="store_true",
        help="Whether to enable profile-guided optimisation.",
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


def configure(
    build_dir: pathlib.Path,
    build_type: str,
    problem: str,
    *additional: list[str],
):
    cwd = pathlib.Path.cwd()
    args = [
        # fmt: off
        build_dir,
        "--buildtype", build_type,
        f"-Dpython.platlibdir={cwd.absolute()}",
        f"-Dproblem={problem}",
        f"-Dstrip={'true' if build_type == 'release' else 'false'}",
        f"-Db_coverage={'true' if build_type != 'release' else 'false'}",
        *additional,
        # fmt: on
    ]

    cmd = "configure" if build_dir.exists() else "setup"
    check_call(["meson", cmd, *args])  # type: ignore


def compile(build_dir: pathlib.Path, verbose: bool):
    args = ["-C", build_dir] + (["--verbose"] if verbose else [])
    check_call(["meson", "compile", *args])  # type: ignore


def install(build_dir: pathlib.Path):
    check_call(["meson", "install", "-C", build_dir])


def build(
    build_dir: pathlib.Path,
    build_type: str,
    problem: str,
    verbose: bool,
    *additional: list[str],
):
    configure(
        build_dir,
        build_type,
        problem,
        *additional,
    )
    compile(build_dir, verbose)
    install(build_dir)


def workload():
    # TODO if and when we actually start using PGO we should probably rethink
    # what the profiling workload needs to be. For example, larger instances
    # are harder to solve, so perhaps we should optimise for those?
    cmds = [
        "pytest",
        "pyvrp --seed 1 tests/data/X-n101-50-k13.vrp --max_runtime 5",
        "pyvrp --seed 2 tests/data/RC208.vrp --max_runtime 5",
    ]

    for cmd in cmds:
        check_call(cmd.split())


def main():
    args = parse_args()
    cwd = pathlib.Path.cwd()
    build_dir = cwd / args.build_dir

    if args.clean or os.environ.get("CIBUILDWHEEL", "0") == "1":
        # Always start from an entirely clean build when building wheels in
        # the CI. Else only do so when expressly asked.
        install_dir = cwd / "pyvrp"
        clean(build_dir, install_dir)

    build_args = (
        build_dir,
        args.build_type,
        args.problem,
        args.verbose,
        *args.additional,
    )

    if args.use_pgo:
        build(*build_args, "-Db_pgo=generate")
        workload()
        build(*build_args, "-Db_pgo=use")
    else:
        build(*build_args)


if __name__ == "__main__":
    main()
