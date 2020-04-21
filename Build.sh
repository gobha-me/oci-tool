#!/bin/env bash

echo "Building oci-inspect"
clang++ -g -std=c++17 -Wall -Iinclude -lssl -lcrypto -o bin/oci-inspect src/bin/oci-inspect.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp

echo "Building oci-sync"
clang++ -g -std=c++17 -Wall -Iinclude -lssl -lcrypto -o bin/oci-sync src/bin/oci-sync.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp yaml/Yaml.cpp