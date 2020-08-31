cmake_minimum_required(VERSION 3.10)

project(OCI_Tools VERSION 0.8.0)

configure_file(include/OCI/version.hpp.in include/OCI/version.hpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

file(GLOB_RECURSE OCI_SRC src/lib/*.cpp)
add_executable(oci-sync src/bin/oci-sync.cpp ${OCI_SRC} yaml/Yaml.cpp)
target_include_directories(oci-sync PUBLIC include /usr/include/botan-2/)
target_link_libraries(oci-sync pthread ssl crypto botan-2)
#find_library(oci-sync NAMES ssl crypto botan-2)
