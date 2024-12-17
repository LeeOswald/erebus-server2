rm -rf build
mkdir -p build
conan install conan/debian-64/conanfile.txt --profile conan/debian-64/conanprofile.debug --output-folder=. --build=missing
conan install conan/debian-64/conanfile.txt --profile conan/debian-64/conanprofile.release --output-folder=. --build=missing
