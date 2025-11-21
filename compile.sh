#!/bin/bash

cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wdeprecated-declarations -DROOT_PATH_ABS=%cd%
(cd build && ninja)