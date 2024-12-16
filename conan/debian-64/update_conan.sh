#!/bin/bash
conan install . --profile conanprofile.debug --output-folder=build --build=missing
conan install . --profile conanprofile.release --output-folder=build --build=missing

