project(
    'pyvrp', 
    'cpp', 
    default_options : [
        'cpp_std=c++20',
        'b_lto=true',  # sets -flto
        'werror=true',  # sets -Werror
        'warning_level=3',  # level 3 sets -Wextra and -Wpedantic
    ]
)

# The following lines define compiler and linker options for the debug build.
# In particular, we also set up debug builds to provide coverage results.
compiler = meson.get_compiler('cpp')

if get_option('buildtype') == 'debug'
    if compiler.has_argument('-fprofile-abs-path')
        # clang does not have this particular flag, so we only add it when
        # compiling with gcc. It's helpful in determining code coverage.
        add_project_arguments('-fprofile-abs-path', language: 'cpp')
    endif

    add_project_arguments('--coverage', language: 'cpp')
    add_project_link_arguments('--coverage', language: 'cpp')
endif

if compiler.has_argument('-Wno-unused-parameter')
    # Ignore unused parameters, because not all operator implementations use 
    # every argument they are given (and that's OK).
    add_project_arguments('-Wno-unused-parameter', language: 'cpp')
endif

if get_option('problem') == 'cvrp'
    # CVRP does not have time windows, so we set a flag that compiles time 
    # window stuff out of the extension modules.
    add_project_arguments('-DVRP_NO_TIME_WINDOWS', language: 'cpp')

    if compiler.has_argument('-Wno-unused-private-field')
        # Ignore unused fields, since a number of time window related fields
        # are unused by the CVRP.
        add_project_arguments('-Wno-unused-private-field', language: 'cpp')
    endif
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
        SRC_DIR / 'ProblemData.cpp',
        SRC_DIR / 'XorShift128.cpp',
        SRC_DIR / 'Individual.cpp',
        SRC_DIR / 'SubPopulation.cpp',
        SRC_DIR / 'crossover' / 'selective_route_exchange.cpp',
        SRC_DIR / 'crossover' / 'crossover.cpp',
        SRC_DIR / 'diversity' / 'broken_pairs_distance.cpp',
        SRC_DIR / 'educate' / 'LocalSearch.cpp',
        SRC_DIR / 'educate' / 'Route.cpp',
        SRC_DIR / 'educate' / 'Node.cpp',
        SRC_DIR / 'educate' / 'MoveTwoClientsReversed.cpp',
        SRC_DIR / 'educate' / 'TwoOpt.cpp',
        SRC_DIR / 'educate' / 'RelocateStar.cpp',
        SRC_DIR / 'educate' / 'SwapStar.cpp',
    ],
    include_directories: INCLUDES,
)

# Next we get the extension dependencies. These are pretty simple: we only
# depend on Python (duh!) and pybind11.
py = import('python').find_installation()
pybind11 = dependency('pybind11', required: false)

if not pybind11.found()
    # Fallback: this looks for the pybind11 header files using pybind11-config.
    # Getting pybind11 using Meson's dependency mechanism is nicer, but that
    # sometimes fails even when pybind11 is actually available on the system.
    message('Could not find pybind11 as a dependency: trying headers.')

    pybind11 = find_program('pybind11-config')
    pybind11 = run_command(pybind11, ['--includes'], check: true)
    pybind11 = pybind11.stdout().split('-I')[-1].strip()
    pybind11 = declare_dependency(include_directories: [pybind11])
endif

assert(pybind11.found(), 'Could not find pybind11!')
dependencies = [py.dependency(), pybind11]

# Extension as [extension name, subdirectory]. Here 'extension name' names the
# eventual module name and the bindings source file, and 'subdirectory' gives 
# the source and installation directories (relative to SRC_DIR and INST_DIR).
extensions = [
    ['Matrix', ''],
    ['CostEvaluator', ''],
    ['ProblemData', ''],
    ['SubPopulation', ''],
    ['TimeWindowSegment', ''],
    ['XorShift128', ''],
    ['Individual', ''],
    ['selective_route_exchange', 'crossover'],
    ['broken_pairs_distance', 'diversity'],
    ['LocalSearch', 'educate'],
    ['Exchange', 'educate'],
    ['MoveTwoClientsReversed', 'educate'],
    ['TwoOpt', 'educate'],
    ['RelocateStar', 'educate'],
    ['SwapStar', 'educate'],
]

foreach extension : extensions  # extension[0] = name, extension[1] = subdir
    message('Going to build ' + extension[0])

    source_dir = '/'.join([SRC_DIR, extension[1]])
    install_dir = '/'.join([INST_DIR, extension[1]])

    py.extension_module(
        '_' + extension[0],  # native extensions are prepended by an underscore
        source_dir / extension[0] + '_bindings.cpp',
        dependencies: dependencies,
        link_with: libcommon,
        install: true,
        subdir: install_dir,
        include_directories: INCLUDES,
    )
endforeach
