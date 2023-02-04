from dataclasses import dataclass
from glob import glob
from pathlib import Path
from typing import List

from pybind11.setup_helpers import (
    ParallelCompile,
    Pybind11Extension,
    naive_recompile,
)

# add_definitions(-Werror -Wall -Wextra -Wpedantic -fPIC -Wno-unused-parameter)
#
# if (CMAKE_BUILD_TYPE MATCHES RELEASE)
#     add_definitions(-O3 -flto)
# else ()
#     # For code coverage on the test builds
#     add_definitions(-fprofile-arcs -ftest-coverage)
#     add_link_options(--coverage)
# endif ()

CPP_DIR = "pyvrp/cpp"


@dataclass
class ExtensionModule:
    package: str
    sources: List[str]

    def as_extension(self) -> Pybind11Extension:
        includes = glob(CPP_DIR + "/**/", recursive=True)
        name = Path(self.sources[0]).stem

        return Pybind11Extension(
            self.package + "." + name,
            # Sort headers and sources to ensure everything's compiled in the
            # same order, every time.
            include_dirs=sorted(includes),
            sources=sorted(self.sources),
            cxx_std=20,
        )


# TODO make this use an actual build system?
modules = [
    ExtensionModule("pyvrp", [f"{CPP_DIR}/Matrix.cpp"]),
    ExtensionModule("pyvrp", [f"{CPP_DIR}/PenaltyManager.cpp"]),
    ExtensionModule("pyvrp", [f"{CPP_DIR}/ProblemData.cpp"]),
    ExtensionModule("pyvrp", [f"{CPP_DIR}/TimeWindowSegment.cpp"]),
    ExtensionModule("pyvrp", [f"{CPP_DIR}/XorShift128.cpp"]),
    ExtensionModule(
        "pyvrp",
        [
            f"{CPP_DIR}/Individual.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
            f"{CPP_DIR}/XorShift128.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.crossover",
        [
            f"{CPP_DIR}/crossover/selective_route_exchange.cpp",
            f"{CPP_DIR}/crossover/crossover.cpp",
            f"{CPP_DIR}/Individual.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
            f"{CPP_DIR}/XorShift128.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.diversity",
        [
            f"{CPP_DIR}/diversity/broken_pairs_distance.cpp",
            f"{CPP_DIR}/Individual.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
            f"{CPP_DIR}/XorShift128.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/LocalSearch.cpp",
            f"{CPP_DIR}/educate/Route.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/Individual.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
            f"{CPP_DIR}/XorShift128.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/Exchange.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/MoveTwoClientsReversed.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/TwoOpt.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/SwapStar.cpp",
            f"{CPP_DIR}/educate/Route.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
        ],
    ),
    ExtensionModule(
        "pyvrp.educate",
        [
            f"{CPP_DIR}/educate/RelocateStar.cpp",
            f"{CPP_DIR}/educate/Exchange.cpp",
            f"{CPP_DIR}/educate/Route.cpp",
            f"{CPP_DIR}/educate/Node.cpp",
            f"{CPP_DIR}/ProblemData.cpp",
            f"{CPP_DIR}/PenaltyManager.cpp",
        ],
    ),
]


def build(setup_kwargs):
    ParallelCompile(default=0, needs_recompile=naive_recompile).install()

    ext_modules = [m.as_extension() for m in modules]
    setup_kwargs.update(dict(ext_modules=ext_modules))
