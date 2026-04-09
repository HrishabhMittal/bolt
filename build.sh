#!/bin/bash
echo ------------ MAKING EVERYTHING -----------------
make -B -j$(nproc) &
cd bvm
make -B -j$(nproc)
cd ..
