#!/bin/bash

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../conan/debian-64/build/build/Release/generators/conan_toolchain.cmake  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
cmake --build . --config Release



