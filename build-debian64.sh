#!/bin/bash

mkdir -p build
conan install conan/debian-64/conanfile.txt --profile conan/debian-64/conanprofile.debug --output-folder=. --build=missing
conan install conan/debian-64/conanfile.txt --profile conan/debian-64/conanprofile.release --output-folder=. --build=missing
pushd build

echo "PWD=[$(pwd)]"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$(pwd)/Release/generators/conan_toolchain.cmake  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
cmake --build . --config Release
popd


