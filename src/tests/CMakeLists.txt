message("Processing t/src/CMakeList.txt")

set(SRC_LIB_DIR ${PROJECT_SOURCE_DIR}/src/lib)

file(GLOB TEMPLATE_SRCS ${CMAKE_CURRENT_LIST_DIR} *.cpp.in)
foreach(TMPL_SRC ${TEMPLATE_SRCS})
  if(NOT IS_DIRECTORY ${TMPL_SRC})
    string(REGEX REPLACE "\\.[^.]*$" "" OUTPUT_NAME ${TMPL_SRC})
    configure_file(${TMPL_SRC} ${OUTPUT_NAME})
  endif()
endforeach()

file(GLOB TEST_SRCS ${CMAKE_CURRENT_LIST_DIR} *.cpp)

foreach(testSrc ${TEST_SRCS})
  if(IS_DIRECTORY ${testSrc})
    continue()
  endif()

  get_filename_component(testName ${testSrc} NAME_WE)
  set(testName "${testName}_test")

  file(STRINGS ${testSrc} testDEPS REGEX "CMAKE_DEP=.*")
  file(STRINGS ${testSrc} testCPP REGEX "CMAKE_CPP_EXTRA=.*")

  if(testCPP)
    string(REPLACE "//CMAKE_CPP_EXTRA=" "" testCPP ${testCPP})
    add_executable(${testName} ${testSrc} ${testCPP})
  else(testCPP)
    add_executable(${testName} ${testSrc})
  endif(testCPP)

  if(testDEPS)
    string(REPLACE "//CMAKE_DEP=" "" testDEPS ${testDEPS})
    target_link_libraries(${testName} ${testDEPS} Catch2::Catch2WithMain)
  else(testDEPS)
    message("NO TEST DEPS in ${testName}")
    target_link_libraries(${testName} Catch2::Catch2WithMain)
  endif(testDEPS)

  add_test(${testName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testName})
endforeach(testSrc)
