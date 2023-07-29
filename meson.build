project(
    'pyvrp',
    'cpp',
    version: run_command('poetry', 'version', '--short', check: true).stdout(),
    default_options: [
        'cpp_std=c++20',
        'b_lto=true',  # sets -flto
        'werror=true',  # sets -Werror
        'warning_level=3',  # level 3 sets -Wextra and -Wpedantic
    ]
)

if get_option('buildtype') == 'debug'
    compiler = meson.get_compiler('cpp')

    if compiler.has_argument('-fprofile-abs-path')
        # clang does not have this particular flag, so we only add it when
        # compiling with gcc. It's helpful in determining code coverage.
        add_project_arguments('-fprofile-abs-path', language: 'cpp')
    endif

    add_project_arguments('--coverage', language: 'cpp')
    add_project_link_arguments('--coverage', language: 'cpp')
endif

if get_option('problem') == 'cvrp'
    # CVRP does not have time windows, so we set a flag that compiles time 
    # window stuff out of the extension modules.
    add_project_arguments('-DPYVRP_NO_TIME_WINDOWS', language: 'cpp')
endif

if get_option('precision') == 'double'  # default is integer
    add_project_arguments('-DPYVRP_DOUBLE_PRECISION', language: 'cpp')
endif

# We first compile a common library that contains all regular, C++ code. This
# is then linked against by the extension modules. We also define source and
# installation directories here, as a shorthand.
INST_DIR = 'pyvrp'
SRC_DIR = 'pyvrp' / 'cpp'
INCLUDES = [include_directories(SRC_DIR)]

libcommon = static_library(
    'common',
    [
        SRC_DIR / 'CostEvaluator.cpp',
        SRC_DIR / 'DynamicBitset.cpp',
        SRC_DIR / 'ProblemData.cpp',
        SRC_DIR / 'RandomNumberGenerator.cpp',
        SRC_DIR / 'Solution.cpp',
        SRC_DIR / 'SubPopulation.cpp',
        SRC_DIR / 'TimeWindowSegment.cpp',
        SRC_DIR / 'crossover' / 'selective_route_exchange.cpp',
        SRC_DIR / 'crossover' / 'crossover.cpp',
        SRC_DIR / 'diversity' / 'broken_pairs_distance.cpp',
        SRC_DIR / 'search' / 'LocalSearch.cpp',
        SRC_DIR / 'search' / 'Route.cpp',
        SRC_DIR / 'search' / 'MoveTwoClientsReversed.cpp',
        SRC_DIR / 'search' / 'TwoOpt.cpp',
        SRC_DIR / 'search' / 'RelocateStar.cpp',
        SRC_DIR / 'search' / 'SwapStar.cpp',
    ],
    include_directories: INCLUDES,
)

# Next we get the extension dependencies.
py = import('python').find_installation()
dependencies = [py.dependency(), dependency('pybind11')]

# Extension as [extension name, subdirectory]. The 'extension name' names the
# eventual module name, and 'subdirectory' gives the source and installation 
# directories (relative to SRC_DIR and INST_DIR).
extensions = [
    ['pyvrp', ''],
    ['crossover', 'crossover'],
    ['diversity', 'diversity'],
    ['search', 'search'],
]

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
        link_with: libcommon,
        install: true,
        subdir: INST_DIR / subdir,
        include_directories: INCLUDES,
    )
endforeach
