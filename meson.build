project('pyvrp', 'cpp', default_options : ['cpp_std=c++20'])

if get_option('buildtype') == 'release'
    add_project_arguments('-O3', language: 'cpp')
    add_project_link_arguments('-flto', language: 'cpp')
endif

if get_option('buildtype') == 'debug'
    add_project_arguments('-fprofile-abs-path', '--coverage', language: 'cpp')
    add_project_link_arguments('--coverage', language: 'cpp')
endif

py_mod = import('python')
py = py_mod.find_installation()

python = py.dependency()
pybind11 = dependency('pybind11')

INSTALL_DIR = meson.source_root() / 'pyvrp'
SRC_DIR = 'pyvrp' / 'cpp'

extensions = [
    ['Matrix', SRC_DIR / 'Matrix.cpp', INSTALL_DIR],
    ['PenaltyManager', SRC_DIR / 'PenaltyManager.cpp', INSTALL_DIR],
    ['ProblemData', SRC_DIR / 'ProblemData.cpp', INSTALL_DIR],
    ['TimeWindowSegment', SRC_DIR / 'TimeWindowSegment.cpp', INSTALL_DIR],
    ['XorShift128', SRC_DIR / 'XorShift128.cpp', INSTALL_DIR],
    [
        'Individual', 
        [
            SRC_DIR / 'Individual.cpp', 
            SRC_DIR / 'ProblemData.cpp', 
            SRC_DIR / 'PenaltyManager.cpp', 
            SRC_DIR / 'XorShift128.cpp'
        ],
        INSTALL_DIR
    ],
    [
        'selective_route_exchange',
        [
            SRC_DIR / 'crossover' / 'selective_route_exchange.cpp',
            SRC_DIR / 'crossover' / 'crossover.cpp',
            SRC_DIR / 'Individual.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
            SRC_DIR / 'XorShift128.cpp', 
        ],
        INSTALL_DIR / 'crossover'
    ],
    [
        'broken_pairs_distance',
        [
            SRC_DIR / 'diversity' / 'broken_pairs_distance.cpp',
            SRC_DIR / 'Individual.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
            SRC_DIR / 'XorShift128.cpp',
        ],
        INSTALL_DIR / 'diversity'
    ],
    [
        'LocalSearch',
        [
            SRC_DIR / 'educate' / 'LocalSearch.cpp',
            SRC_DIR / 'educate' / 'Route.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'Individual.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
            SRC_DIR / 'XorShift128.cpp',
        ],
        INSTALL_DIR / 'educate'
    ],
    [
        'Exchange',
        [
            SRC_DIR / 'educate' / 'Exchange.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
        ],
        INSTALL_DIR / 'educate'
    ],
    [
        'MoveTwoClientsReversed',
        [
            SRC_DIR / 'educate' / 'MoveTwoClientsReversed.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
        ],
        INSTALL_DIR / 'educate'
    ],
    [
        'TwoOpt',
        [
            SRC_DIR / 'educate' / 'TwoOpt.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
        ],
        INSTALL_DIR / 'educate'
    ],
    [
        'RelocateStar',
        [
            SRC_DIR / 'educate' / 'RelocateStar.cpp',
            SRC_DIR / 'educate' / 'Exchange.cpp',
            SRC_DIR / 'educate' / 'Route.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
        ],
        INSTALL_DIR / 'educate'
    ],
    [
        'SwapStar',
        [
            SRC_DIR / 'educate' / 'SwapStar.cpp',
            SRC_DIR / 'educate' / 'Route.cpp',
            SRC_DIR / 'educate' / 'Node.cpp',
            SRC_DIR / 'ProblemData.cpp',
            SRC_DIR / 'PenaltyManager.cpp',
        ],
        INSTALL_DIR / 'educate'
    ]
]

foreach extension : extensions
    py.extension_module(
        extension[0],
        extension[1],
        dependencies : [pybind11, python],
        install : true,
        install_dir: extension[2],
        include_directories: [
            include_directories(SRC_DIR),
        ],
        cpp_args: [
            '-Werror',
            '-Wall',
            '-Wextra',
            '-Wpedantic',
            # Ignore unused parameters, because not all LS operators use every
            # argument they are given.
            '-Wno-unused-parameter'
        ],
    )
endforeach
