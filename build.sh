#!/bin/sh

cd ./cl
./compose.sh
cd ..

cd ./build/
cmake ..
make
