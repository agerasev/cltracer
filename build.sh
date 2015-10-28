#!/bin/sh

cd ./cl
./compose.sh
cd ..

mkdir -p ./build/
cd ./build/
cmake ..
make
