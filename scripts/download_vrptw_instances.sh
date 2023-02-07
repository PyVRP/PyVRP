#!/usr/bin/env bash

# Exit immediately if any of the commands in this script fail.
set -e

mkdir -p data/vrptw
cd data/vrptw

echo "Downloading Solomon instances"
wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-Solomon.tgz"
tar -xvf "Vrp-Set-Solomon.tgz"
mv Vrp-Set-Solomon Solomon
rm "Vrp-Set-Solomon.tgz"

echo "Downloading Gehring & Homberger instances"
wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz"
tar -xvf "Vrp-Set-HG.tgz" --strip-components=1
mv Vrp-Set-HG GH
rm "Vrp-Set-HG.tgz"

cd ../..
