#!/usr/bin/env bash

# This script generates type stubs for the binary extensions using mypy's
# stubgen tool.

set -e;  # exit immediately if any command fails

_msg="
Generating new type stubs might overwrite manual additions, like corrections 
and docstrings. Please pass in the '--force' flag if you are really sure you
want to do this!
"

if [[ $* != *--force* ]]  # sanity check to ensure no data is lost
then
    echo $_msg;
    exit;
fi


for f in `find pyvrp -type f -name "*.so"`;  # TODO is it always *.so?
do
  dir=$(dirname  "$f");
  file=$(basename "$f");
  libname=${file%%.*}

  echo "-- Generating type stubs for $libname";
  (cd $dir && poetry run stubgen --parse-only -o . -m "$libname")
done
