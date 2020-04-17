BUILD
clang++ -g -std=c++17 -Wall -Iinclude -lssl -lcrypto -o bin/oci-sync src/bin/oci-sync.cpp src/lib/OCI/*.cpp src/lib/OCI/Extensions/*.cpp src/lib/OCI/Registry/*.cpp  yaml/Yaml.cpp

CLANG-TIDY
clang-tidy src/bin/oci-sync.cpp -- -Iinclude --std=c++17
