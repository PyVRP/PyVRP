project(
    'pyvrp',
    'cpp',
    version: run_command(
        'awk',
        # Matches first line with 'version = "XXX"', and extracts "XXX".
        '/version/ { print $3; exit }',
        'pyproject.toml',
        check: true,
    ).stdout().replace('"', ''),  # turns "XXX" into just XXX
    default_options: [
        'cpp_std=c++20',
        'b_ndebug=if-release',  # disables asserts in release builds
        'b_lto=true',  # sets -flto
        'werror=true',  # sets -Werror
        'warning_level=3',  # level 3 sets -Wextra and -Wpedantic
    ]
)

compiler = meson.get_compiler('cpp')
if compiler.has_argument('-Wno-dangling-reference')
    # This is a new diagnostic in GCC 13 and up, but it is a bit buggy right
    # now and issues false positives for some of our code. See #656 and #657
    # for details.
    add_project_arguments('-Wno-dangling-reference', language: 'cpp')
endif

if compiler.has_argument('-Wno-stringop-overflow')
    # This diagnostic issues false positives for some of our code (see #1063).
    add_project_link_arguments('-Wno-stringop-overflow', language: 'cpp')
endif

# Logging configuration and spdlog dependency.
if get_option('buildtype') == 'debug'
    # Log everything in debug builds.
    add_project_arguments('-DPYVRP_LOG_LEVEL=0', language: 'cpp')
else
    # Only log INFO and up in release builds.
    add_project_arguments('-DPYVRP_LOG_LEVEL=2', language: 'cpp')
endif

spdlog = dependency(
    'spdlog',
    default_options: ['std_format=enabled'],
    static: true,
)

# We first compile static libraries that contains all regular, C++ code, one
# per extension module. The C++ libraries depend on spdlog. They are linked
# against when we compile the actual Python extension modules down below. We
# also define the top-level source directory here, for convenience.
SRC_DIR = 'pyvrp' / 'cpp'
INCLUDES = include_directories(SRC_DIR)

libpyvrp = static_library(
    'pyvrp',
    [
        SRC_DIR / 'Activity.cpp',
        SRC_DIR / 'CostEvaluator.cpp',
        SRC_DIR / 'DynamicBitset.cpp',
        SRC_DIR / 'ProblemData.cpp',
        SRC_DIR / 'RandomNumberGenerator.cpp',
        SRC_DIR / 'Route.cpp',
        SRC_DIR / 'Solution.cpp',
        SRC_DIR / 'LoadSegment.cpp',
        SRC_DIR / 'DurationSegment.cpp',
    ],
    dependencies: spdlog,
    include_directories: INCLUDES,
)

libsearch = static_library(
    'search',
    [
        SRC_DIR / 'search' / 'InsertOptional.cpp',
        SRC_DIR / 'search' / 'LocalSearch.cpp',
        SRC_DIR / 'search' / 'neighbourhood.cpp',
        SRC_DIR / 'search' / 'PerturbationManager.cpp',
        SRC_DIR / 'search' / 'Route.cpp',
        SRC_DIR / 'search' / 'RelocateWithDepot.cpp',
        SRC_DIR / 'search' / 'RemoveAdjacentDepot.cpp',
        SRC_DIR / 'search' / 'RemoveOptional.cpp',
        SRC_DIR / 'search' / 'ReplaceGroup.cpp',
        SRC_DIR / 'search' / 'ReplaceOptional.cpp',
        SRC_DIR / 'search' / 'SearchSpace.cpp',
        SRC_DIR / 'search' / 'Solution.cpp',
        SRC_DIR / 'search' / 'SwapTails.cpp',
    ],
    dependencies: spdlog,
    include_directories: INCLUDES,
    link_with: libpyvrp,
)

# Declare as dependency so PyVRP can be easily used as a Meson subproject.
pyvrp_dep = declare_dependency(
    include_directories: INCLUDES,
    link_with: [libpyvrp, libsearch],
    dependencies: spdlog,
)

# Python and pybind11 are needed to build extension modules. When used as a
# subproject, these may be absent - in that case we skip building extensions.
py = import('python').find_installation(required: not meson.is_subproject())
pybind11 = dependency('pybind11', required: not meson.is_subproject())
if py.found() and pybind11.found()
    # Extension dependencies: Python itself, pybind11, and spdlog.
    dependencies = [
        py.dependency(),
        pybind11,
        spdlog,
    ]

    # Extension as [extension name, subdirectory, core C++ library]. The
    # extension name names the eventual module name, subdirectory gives the
    # source and installation directories (relative to top-level directories),
    # and the core C++ library is one of the static libraries we defined above.
    extensions = [
        ['pyvrp', '', libpyvrp],
        ['search', 'search', libsearch],
    ]

    foreach extension : extensions
        rawname = extension[0]
        name = '_' + rawname
        subdir = extension[1]

        message('Going to build extension module ' + subdir / name + '.')

        # Specify a custom target that generates the documentation header for
        # this extension, of the form "<name>_docs.h". It is generated from
        # headers in the relevant source directory - which the command below
        # grabs for us.
        doc_glob = f'import glob; print(*glob.glob("@SRC_DIR@/@subdir@/*.h"))'
        doc_cmd = run_command(py.full_path(), '-c', doc_glob, check: true)
        doc_input_headers = doc_cmd.stdout().split()
        doc_output_header = custom_target(
            'docs for ' + name,
            output: rawname + '_docs.h',
            input: ['buildtools/extract_docstrings.py'] + doc_input_headers,
            command: [py.full_path(), '@INPUT@', '@OUTPUT@'],
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
else
    message('Python or pybind11 not found - skipping extension modules.')
endif
