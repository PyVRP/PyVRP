import skbuild


def build(kwargs):
    skbuild.setup(
        cmake_source_dir="hgs",
        cmake_install_dir="build",
        cmake_args=[
            "-G=Unix Makefiles",  # TODO switch to Ninja?
        ],
        **kwargs
    )
