mkdir build
conan install conan\win64-vc143\conanfile.txt --profile conan\win64-vc143\conanprofile.debug --output-folder=. --build=missing
conan install conan\win64-vc143\conanfile.txt --profile conan\win64-vc143\conanprofile.release --output-folder=. --build=missing
pushd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=generators\conan_toolchain.cmake  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
cmake --build . --config Release
popd
