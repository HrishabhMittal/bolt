#!/bin/bash
echo ------------ RUNNING ON EXAMPLE --------------
cd build
./boltc ../$1 main
../bvm/build/stdprint fmt
../bvm/build/objdump main
time ../bvm/build/bvm main print
rm main print
