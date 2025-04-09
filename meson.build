project(
    'pyvrp',
    'cpp',
    version: run_command('poetry', 'version', '--short', check: true).stdout(),
    default_options: [
        'cpp_std=c++20',
        'b_ndebug=if-release',  # disables asserts in release builds
        'b_lto=true',  # sets -flto
        'werror=true',  # sets -Werror
        'warning_level=3',  # level 3 sets -Wextra and -Wpedantic
    ]
)

if get_option('problem') == 'cvrp'
    # CVRP does not have time windows, so we set a flag that compiles time 
    # window stuff out of the extension modules.
    add_project_arguments('-DPYVRP_NO_TIME_WINDOWS', language: 'cpp')
endif

compiler = meson.get_compiler('cpp')
if compiler.has_argument('-Wno-dangling-reference')
    # This is a new diagnostic in GCC 13 and up, but it is a bit buggy right
    # now and issues false positives for some of our code. See #656 and #657
    # for details.
    add_project_arguments('-Wno-dangling-reference', language: 'cpp')
endif

# We first compile static libraries that contains all regular, C++ code, one
# per extension module. These are linked against when we compile the actual 
# Python extension modules down below. We also define the top-level source 
# directory here, for convenience.
SRC_DIR = 'pyvrp' / 'cpp'
INCLUDES = include_directories(SRC_DIR)

libpyvrp = static_library(
    'pyvrp',
    [
        SRC_DIR / 'CostEvaluator.cpp',
        SRC_DIR / 'DynamicBitset.cpp',
        SRC_DIR / 'ProblemData.cpp',
        SRC_DIR / 'RandomNumberGenerator.cpp',
        SRC_DIR / 'Trip.cpp',
        SRC_DIR / 'Route.cpp',
        SRC_DIR / 'Solution.cpp',
        SRC_DIR / 'SubPopulation.cpp',
        SRC_DIR / 'LoadSegment.cpp',
        SRC_DIR / 'DurationSegment.cpp',
    ],
    include_directories: INCLUDES,
)

libcrossover = static_library(
    'crossover',
    [
        SRC_DIR / 'crossover' / 'ordered_crossover.cpp',
        SRC_DIR / 'crossover' / 'selective_route_exchange.cpp',
    ],
    include_directories: INCLUDES,
    link_with: libpyvrp,
)

libdiversity = static_library(
    'diversity',
    [
        SRC_DIR / 'diversity' / 'broken_pairs_distance.cpp',
    ],
    include_directories: INCLUDES,
    link_with: libpyvrp,
)

libsearch = static_library(
    'search',
    [
        SRC_DIR / 'search' / 'LocalSearch.cpp',
        SRC_DIR / 'search' / 'Route.cpp',
        SRC_DIR / 'search' / 'primitives.cpp',
        SRC_DIR / 'search' / 'SwapRoutes.cpp',
        SRC_DIR / 'search' / 'SwapStar.cpp',
        SRC_DIR / 'search' / 'SwapTails.cpp',
        SRC_DIR / 'search' / 'TripRelocate.cpp',
    ],
    include_directories: INCLUDES,
    link_with: libpyvrp,
)

librepair = static_library(
    'repair',
    [
        SRC_DIR / 'repair' / 'greedy_repair.cpp',
        SRC_DIR / 'repair' / 'nearest_route_insert.cpp',
        SRC_DIR / 'repair' / 'repair.cpp',
    ],
    include_directories: INCLUDES,
    link_with: [libpyvrp, libsearch],  # uses search to evaluate repair moves
)

# Extension as [extension name, subdirectory, core C++ library]. The extension
# name names the eventual module name, subdirectory gives the source and
# installation directories (relative to top-level directories), and the core 
# C++ library is one of the static libraries we defined above.
extensions = [
    ['pyvrp', '', libpyvrp],
    ['crossover', 'crossover', libcrossover],
    ['diversity', 'diversity', libdiversity],
    ['repair', 'repair', librepair],
    ['search', 'search', libsearch],
]

# Extension dependencies: Python itself, and pybind11.
py = import('python').find_installation()
dependencies = [py.dependency(), dependency('pybind11')]

foreach extension : extensions
    rawname = extension[0]
    name = '_' + rawname
    subdir = extension[1]

    message('Going to build extension module ' + subdir / name + '.')

    # Specify a custom target that generates the documentation header for this
    # extension, of the form "<name>_docs.h". It is generated from headers in
    # the relevant source directory - which the command below grabs for us.
    doc_glob = f'import glob; print(*glob.glob("@SRC_DIR@/@subdir@/*.h"))'
    doc_cmd = run_command('python', '-c', doc_glob, check: true)
    doc_input_headers = doc_cmd.stdout().split()
    doc_output_header = custom_target(
        'docs for ' + name,
        output: rawname + '_docs.h',
        input: ['extract_docstrings.py'] + doc_input_headers,
        command: ['python', '@INPUT@', '@OUTPUT@'],
        depend_files: [SRC_DIR / subdir / 'bindings.cpp'],
    )

    # Register extension module to build.
    py.extension_module(
        name,
        [SRC_DIR / subdir / 'bindings.cpp', doc_output_header],
        dependencies: dependencies,
        link_with: extension[2],
        install: true,
        subdir: 'pyvrp' / subdir,
        include_directories: INCLUDES,
    )
endforeach
