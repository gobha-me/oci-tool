cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/version.cmake)

if (NOT CMAKE_TOOLCHAIN_FILE)
  # If RHEL / CentOS / Rocky support is needed submit a patch for the toolchain
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain/default.cmake)
endif()

get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

project(${ProjectId}
  VERSION ${VERSION}
  LANGUAGES CXX
)

configure_file(include/OCI/version.hpp.in include/OCI/version.hpp)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies.cmake)

include_directories( include )

# TODO: Split bin and lib to individual CMakeLists
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)

option(${PROJECT_NAME}_TESTS "ENABLE TESTS" ON)

if (${PROJECT_NAME}_TESTS)
  include(CTest)
  enable_testing()

  # TODO: change dir structure later
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/tests)
endif(${PROJECT_NAME}_TESTS)
