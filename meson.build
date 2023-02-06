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
    # every argument they are given.
    add_project_arguments('-Wno-unused-parameter', language: 'cpp')
endif

# Next we get the extension dependencies. These are pretty simple: we only
# depend on Python (duh!) and pybind11.
py = import('python').find_installation()
pybind11 = dependency('pybind11')

if not pybind11.found()
    # Fallback: this looks for the pybind11 header files using pybind11-config.
    # Getting pybind11 using Meson's dependency mechanism is nicer, but that
    # sometimes fails even when pybind11 is actually available on the system.
    pybind11 = find_program('pybind11-config')
    pybind11 = run_command(pybind11, ['--includes'], check: true)
    pybind11 = pybind11.stdout().split('-I')[-1].strip()
    pybind11 = declare_dependency(include_directories: [pybind11])
endif

assert(pybind11.found(), 'Could not find pybind11!')

# Now that everything is in place, let's defines the extensions to be built by
# Meson. These are given as tuples on the following lines. The tuples look like
# (extension name, source files, install dir), where 'extension name' is the
# eventual module name, the source files name cpp source files (in pyvrp/cpp), 
# and 'install dir' gives the directory where the extension should be placed.

INSTALL_DIR = meson.source_root() / 'pyvrp'
SRC_DIR = 'pyvrp' / 'cpp'

libcommon = static_library(
    'common',
    [
        SRC_DIR / 'PenaltyManager.cpp',
        SRC_DIR / 'ProblemData.cpp',
        SRC_DIR / 'XorShift128.cpp',
        SRC_DIR / 'Individual.cpp',
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
    dependencies: [py.dependency(), pybind11],
    include_directories: [include_directories(SRC_DIR)],
)

extensions = [
    ['Matrix', SRC_DIR / 'Matrix_bindings.cpp', INSTALL_DIR],
    ['PenaltyManager', SRC_DIR / 'PenaltyManager_bindings.cpp', INSTALL_DIR],
    ['ProblemData', SRC_DIR / 'ProblemData_bindings.cpp', INSTALL_DIR],
    ['TimeWindowSegment', SRC_DIR / 'TimeWindowSegment_bindings.cpp', INSTALL_DIR],
    ['XorShift128', SRC_DIR / 'XorShift128_bindings.cpp', INSTALL_DIR],
    ['Individual', SRC_DIR / 'Individual_bindings.cpp', INSTALL_DIR],
    ['selective_route_exchange', SRC_DIR / 'crossover' / 'selective_route_exchange_bindings.cpp', INSTALL_DIR / 'crossover'],
    ['broken_pairs_distance', SRC_DIR / 'diversity' / 'broken_pairs_distance_bindings.cpp', INSTALL_DIR / 'diversity'],
    ['LocalSearch', SRC_DIR / 'educate' / 'LocalSearch_bindings.cpp', INSTALL_DIR / 'educate'],
    ['Exchange', SRC_DIR / 'educate' / 'Exchange_bindings.cpp', INSTALL_DIR / 'educate'],
    ['MoveTwoClientsReversed', SRC_DIR / 'educate' / 'MoveTwoClientsReversed_bindings.cpp', INSTALL_DIR / 'educate'],
    ['TwoOpt', SRC_DIR / 'educate' / 'TwoOpt_bindings.cpp', INSTALL_DIR / 'educate'],
    ['RelocateStar', SRC_DIR / 'educate' / 'RelocateStar_bindings.cpp', INSTALL_DIR / 'educate'],
    ['SwapStar', SRC_DIR / 'educate' / 'SwapStar_bindings.cpp', INSTALL_DIR / 'educate'],
]

foreach extension : extensions
    message('Going to build ' + extension[0])

    py.extension_module(
        extension[0],
        extension[1],
        dependencies : [py.dependency(), pybind11],
        link_with: libcommon,
        install : true,
        install_dir: extension[2],
        include_directories: [
            include_directories(SRC_DIR),
        ],
    )
endforeach
