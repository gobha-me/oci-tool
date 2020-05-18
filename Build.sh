#!/bin/env bash

echo "Building oci-inspect"
#clang++ -g -std=c++17 -Wall -pthread -Iinclude -I/usr/include/botan-2 -lssl -lcrypto -lbotan-2 -o bin/oci-inspect src/bin/oci-inspect.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp

echo "Building oci-sync"
clang++ -g -D_GLIBCXX_DEBUG  -std=c++17 -Wall -pthread -Iinclude -I/usr/include/botan-2 -lssl -lcrypto -lbotan-2 -o bin/oci-sync src/bin/oci-sync.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp yaml/Yaml.cpp
#clang++ -O2 -g -D_GLIBCXX_DEBUG -fsanitize=thread -fno-omit-frame-pointer -std=c++17 -Wall -pthread -Iinclude -I/usr/include/botan-2 -lssl -lcrypto -lbotan-2 -o bin/oci-sync src/bin/oci-sync.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp yaml/Yaml.cpp
#clang++ -O3 -std=c++17 -Wall -pthread -Iinclude -I/usr/include/botan-2 -lssl -lcrypto -lbotan-2 -o bin/oci-sync src/bin/oci-sync.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp yaml/Yaml.cpp
