file(GLOB_RECURSE OCI_SRC ${CMAKE_CURRENT_LIST_DIR}/lib/*.cpp)

add_executable(oci-sync
  ${CMAKE_CURRENT_LIST_DIR}/bin/oci-sync.cpp
  ${OCI_SRC}
  ${CMAKE_SOURCE_DIR}/yaml/Yaml.cpp
)

#target_include_directories(oci-sync PUBLIC include)

target_link_libraries(oci-sync
  ssl
  pthread
  crypto
)
