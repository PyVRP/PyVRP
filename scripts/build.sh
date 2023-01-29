#!/usr/bin/env bash

BUILD_DIR=build
PKG_LIB_DIR=pyvrp/_lib

echo "1) Writing build scripts"
poetry run cmake -Spyvrp/cpp -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE="${1-Release}"

echo "2) Compiling libraries"
poetry run cmake --build "$BUILD_DIR"

echo "3) Moving libraries"
mkdir -p "$PKG_LIB_DIR";

for f in "$BUILD_DIR"/lib/*.so;  # TODO is it always *.so?
do
  file=$(basename "$f");
  libname=${file%%.*}

  echo "-- Copying compiled library $libname into $PKG_LIB_DIR";
  cp "$f" "$PKG_LIB_DIR"/"$file";

  echo "-- Generating type stubs for $libname";
  (cd $PKG_LIB_DIR && poetry run stubgen --parse-only -o . -m "$libname")
done
