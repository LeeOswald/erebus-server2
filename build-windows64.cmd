mkdir build-win64-realese

cmake -B build -S . -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=generators\conan_toolchain.cmake  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
make --build . --config Release
cmake --build .
