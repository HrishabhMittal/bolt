#!/bin/bash
echo ------------ MAKING COMPILER -----------------
make -B -j$(nproc)
echo ------------ MAKING VM "&" UTILS -------------
cd bvm
make -B -j$(nproc)
cd ..
