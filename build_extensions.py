import argparse
import pathlib
from subprocess import call


def parse_args():
    parser = argparse.ArgumentParser(prog="builder")

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
        "--regen_stubs",
        action="store_true",
        help="""
        Whether to regenerate the MyPy type stubs as well. Default False, since
        this can overwrite manual improvements and docstrings.
        """,
    )
    parser.add_argument(
        "--additional",
        nargs=argparse.REMAINDER,
        default=[],
        help="Extra Meson configuration options (passed verbatim to Meson).",
    )

    return parser.parse_args()


def build(
    build_dir: str,
    build_type: str,
    problem: str,
    regen_stubs: bool,
    additional: list[str],
    **kwargs,
):
    cwd = pathlib.Path.cwd()
    build_loc = cwd / build_dir
    install_loc = cwd / "pyvrp"

    args = [
        # fmt: off
        "--buildtype", build_type,
        f"-Dpython.platlibdir={cwd.absolute()}",
        f"-Dproblem={problem}",
        *additional,
        # fmt: on
    ]

    if not build_loc.exists():
        call(["meson", "setup", build_loc, *args])
    else:
        call(["meson", "configure", build_loc, *args])

    call(["meson", "compile", "-C", build_loc])
    call(["meson", "install", "-C", build_loc])

    if regen_stubs:
        for extension in install_loc.rglob("*.so"):  # TODO only *.so?
            ext_dir = extension.parent
            ext_name, _ = extension.name.split(".", maxsplit=1)

            call(
                ["stubgen", "--parse-only", "-o", ".", "-m", ext_name],
                cwd=ext_dir,
            )


if __name__ == "__main__":
    args = parse_args()
    build(**vars(args))
