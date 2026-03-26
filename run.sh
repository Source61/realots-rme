#!/bin/bash

mkdir build -p &&
cd build &&
cmake .. &&
make -j `nproc` &&
cp rme ../ &&
cd ../ &&
./rme
