import argparse
import pathlib

import docblock

# The macros below are shamelessly taken from pybind11_mkdocs, here:
# https://github.com/pybind/pybind11_mkdoc.
_PREFIX = """
// Auto-generated during compilation. Do not edit.

#define __EXPAND(x) x
#define __COUNT(_1, _2, _3, _4, _5, _6, _7, COUNT, ...) COUNT
#define __VA_SIZE(...) __EXPAND(__COUNT(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0))
#define __CAT1(a, b) a ## b
#define __CAT2(a, b) __CAT1(a, b)
#define __DOC1(n1) __doc_##n1
#define __DOC2(n1, n2) __doc_##n1##_##n2
#define __DOC3(n1, n2, n3) __doc_##n1##_##n2##_##n3
#define __DOC4(n1, n2, n3, n4) __doc_##n1##_##n2##_##n3##_##n4
#define __DOC5(n1, n2, n3, n4, n5) __doc_##n1##_##n2##_##n3##_##n4##_##n5
#define __DOC6(n1, n2, n3, n4, n5, n6) \
    __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6
#define __DOC7(n1, n2, n3, n4, n5, n6, n7) \
    __doc_##n1##_##n2##_##n3##_##n4##_##n5##_##n6##_##n7
#define DOC(...) \
    __EXPAND(__EXPAND(__CAT2(__DOC, __VA_SIZE(__VA_ARGS__)))(__VA_ARGS__))

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
"""

_SUFFIX = """
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
"""


def parse_args():
    parser = argparse.ArgumentParser(prog="extract_docstrings")

    parser.add_argument(
        "bindings_loc",
        type=pathlib.Path,
        help="Location of the bindings.cpp file to compile docstrings for.",
    )
    parser.add_argument(
        "doc_header_name",
        help="Header name to write docstrings into.",
    )

    return parser.parse_args()


def to_cpp_string(name, docstrings):
    name = "__doc_" + name.replace("::", "_")

    if len(docstrings) == 1:
        doc = docstrings[0]
        return f'char const *{name} = R"doc(\n{doc}\n)doc";'

    return "\n".join(
        f'char const *{name}_{ctr} = R"doc(\n{docstring}\n)doc";'
        for ctr, docstring in enumerate(docstrings, 1)
    )


def main():
    args = parse_args()
    src_dir = args.bindings_loc.parent
    parsed = {}

    for header in src_dir.glob("*.h"):
        parsed.update(docblock.parse_file(header))

    file = args.doc_header_name
    docs = "\n".join(map(lambda item: to_cpp_string(*item), parsed.items()))

    with open(file, "w") as fh:
        for section in [_PREFIX, docs, _SUFFIX]:
            fh.write(section)
            fh.write("\n")


if __name__ == "__main__":
    main()
