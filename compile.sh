#!/bin/bash

cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wdeprecated-declarations -DROOT_PATH_ABS=%cd% -DENABLE_ASAN=On
cmake --build build --config Debug
