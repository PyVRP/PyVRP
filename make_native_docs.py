import argparse
import pathlib
from collections import defaultdict

import docblock


def parse_args():
    parser = argparse.ArgumentParser(prog="make_native_docs")

    parser.add_argument(
        "src_dir",
        help="Directory containing source header files.",
        type=pathlib.Path,
    )

    return parser.parse_args()


def main():
    args = parse_args()
    docstrings = defaultdict(dict)

    for header in args.src_dir.glob("**/*.h"):
        parsed = docblock.parse_file(header)
        docstrings[header.parent].update(parsed)

    print(docstrings)


if __name__ == "__main__":
    main()
