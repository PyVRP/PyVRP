"""
Builds the native extensions.
"""

import argparse
import os
import pathlib
import sys
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
    *additional: list[str],
):
    cwd = pathlib.Path.cwd()
    # fmt: off
    args = [
        build_dir,
        "--buildtype", build_type,
        f"-Dpython.platlibdir={cwd.absolute()}",
        f"-Dstrip={'true' if build_type == 'release' else 'false'}",
        f"-Db_coverage={'true' if build_type != 'release' else 'false'}",
        *additional,
    ]
    # fmt: on

    cmd = "configure" if build_dir.exists() else "setup"
    check_call(["meson", cmd, *args])  # type: ignore


def compile(build_dir: pathlib.Path, verbose: bool):
    args = ["-C", build_dir] + (["--verbose"] if verbose else [])
    check_call(["meson", "compile", *args])  # type: ignore


def install(build_dir: pathlib.Path):
    check_call(["meson", "install", "-C", build_dir, "--skip-subprojects"])


def build(
    build_dir: pathlib.Path,
    build_type: str,
    verbose: bool,
    *additional: list[str],
):
    configure(build_dir, build_type, *additional)
    compile(build_dir, verbose)
    install(build_dir)


def workload(build_dir: pathlib.Path):
    cmds = [
        "pytest -n0",
        "pyvrp --seed 1 tests/data/X-n101-50-k13.vrp --max_runtime 5",
        "pyvrp --seed 2 tests/data/RC208.vrp --max_runtime 5",
        "pyvrp --seed 3 tests/data/lrc206.vrp --max_runtime 5",
    ]
    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = str(build_dir / "%m-%p.profraw")
    env.pop("GCOV_PREFIX", None)
    env.pop("GCOV_PREFIX_STRIP", None)

    for cmd in cmds:
        check_call(cmd.split(), env=env)


def remove_profiles(build_dir: pathlib.Path):
    for pattern in ("*.gcda", "*.profdata", "*.profraw"):
        for profile in build_dir.rglob(pattern):
            profile.unlink()


def merge_profiles(build_dir: pathlib.Path):
    profiles = sorted(build_dir.glob("*.profraw"))

    if not profiles:
        return

    llvm_profdata = os.environ.get("LLVM_PROFDATA")
    if llvm_profdata is not None:
        command = [llvm_profdata]
    elif sys.platform == "darwin":
        command = ["xcrun", "llvm-profdata"]
    else:
        command = ["llvm-profdata"]

    output = build_dir / "default.profdata"
    check_call(
        [
            *command,
            "merge",
            f"-output={output}",
            *map(str, profiles),
        ]
    )


def main():
    args = parse_args()
    cwd = pathlib.Path.cwd()
    build_dir = cwd / args.build_dir

    if args.clean:
        install_dir = cwd / "pyvrp"
        clean(build_dir, install_dir)

    build_args = (
        build_dir,
        args.build_type,
        args.verbose,
        *args.additional,
    )

    if args.use_pgo:
        # GCC requires profile data for every compiled source file. Some
        # sources in the static spdlog dependency are not linked into the
        # extensions, so no profile data can be generated for them.
        spdlog_arg = "-Dspdlog:b_pgo=off"
        build(*build_args, "-Db_pgo=generate", spdlog_arg)
        remove_profiles(build_dir)
        workload(build_dir)
        merge_profiles(build_dir)
        build(*build_args, "-Db_pgo=use", spdlog_arg)
    else:
        build(*build_args)


if __name__ == "__main__":
    main()
