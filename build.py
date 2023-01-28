import skbuild


def build(kwargs):
    skbuild.setup(
        cmake_source_dir="pyvrp/cpp",
        cmake_install_dir="pyvrp",
        cmake_args=[
            "-G=Unix Makefiles",  # TODO switch to Ninja?
        ],
        **kwargs
    )
