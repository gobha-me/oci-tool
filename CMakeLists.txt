cmake_minimum_required(VERSION 2.8)

if ( ${CMAKE_MAJOR_VERSION} GREATER "3" )
  project(OCI_Tools VERSION 0.8.0)
  configure_file(include/OCI/version.hpp.in include/OCI/version.hpp)
else()
  project(OCI_Tools)
endif()

if (EXISTS "/usr/bin/clang++")
  set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_FLAGS             "-Wall")
elseif(EXISTS "/opt/rh/devtoolset-9/root/usr/bin/g++")
  set(CMAKE_CXX_COMPILER "/opt/rh/devtoolset-9/root/usr/bin/g++")
  set(CMAKE_CXX_FLAGS    "-Wall -std=c++17")
endif()

#set(CMAKE_CXX_FLAGS_DEBUG          "-O1 -fsanitize=thread -g -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE OCI_SRC src/lib/*.cpp)
add_executable(oci-sync src/bin/oci-sync.cpp ${OCI_SRC} yaml/Yaml.cpp)

if ( ${CMAKE_MAJOR_VERSION} EQUAL "3" OR ${CMAKE_MAJOR_VERSION} GREATER "3" )
  include_directories( include )
  #target_include_directories(oci-sync PUBLIC include)

  target_link_libraries(oci-sync ssl pthread crypto)
else()
  include_directories( include )

  if ( EXISTS "/usr/include/openssl11" )
    include_directories( BEFORE SYSTEM "/usr/include/openssl11" )

    target_link_libraries(oci-sync /usr/lib64/libssl.so.1.1 /usr/lib64/libcrypto.so.1.1 pthread crypto)
  endif()
endif()

enable_testing()
add_subdirectory(src/tests)
