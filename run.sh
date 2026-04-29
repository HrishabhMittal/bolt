#!/bin/bash
set -e
echo ------------ RUNNING ON EXAMPLE --------------
cd build
./boltc ../$1 main
../bvm/build/stdprint fmt
# ../bvm/build/objdump main
# samply record ../bvm/build/bvm main print
# flamegraph ../bvm/build/bvm main print
echo NO OPTIMISATIONS
../bvm/build/bvm main print
../bvm/build/optimiser main main
echo WITH OPTIMISATIONS
../bvm/build/bvm main print
rm main print
