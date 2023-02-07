#!/usr/bin/env bash

# Exit immediately if any of the commands in this script fail.
set -e

mkdir -p data/vrptw
cd data/vrptw

# echo "Downloading Solomon instances"
# wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-Solomon.tgz"
# tar -xvf "Vrp-Set-Solomon.tgz"
# mv Vrp-Set-Solomon Solomon
# rm "Vrp-Set-Solomon.tgz"

# echo "Downloading Gehring & Homberger instances"
# wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz"
# tar -xvf "Vrp-Set-HG.tgz" --strip-components=1
# mv Vrp-Set-HG GH
# rm "Vrp-Set-HG.tgz"

mkdir -p ORTEC
cd ORTEC
wget "https://github.com/ortec/euro-neurips-vrp-2022-quickstart/raw/main/instances/ORTEC-VRPTW-ASYM-00c5356f-d1-n258-k12.txt"
wget "https://github.com/ortec/euro-neurips-vrp-2022-quickstart/raw/main/instances/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt"
cd ..

cd ../..
