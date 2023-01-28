#!/usr/bin/env bash

BUILD_DIR=build
PKG_LIB_DIR=pyvrp/_lib

echo "1) Writing build scripts"
cmake -Spyvrp/cpp -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE="${1-Release}"

echo "2) Compiling libraries"
cmake --build "$BUILD_DIR"

echo "3) Moving libraries"
mkdir -p "$PKG_LIB_DIR";

for f in "$BUILD_DIR"/lib/*.so;
do
  echo "-- Copying compiled library $f into $PKG_LIB_DIR";
  cp "$f" "$PKG_LIB_DIR"/"$(basename "$f")";
done
