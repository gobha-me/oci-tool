message("Processing t/src/CMakeList.txt")

set(SRC_LIB_DIR ${PROJECT_SOURCE_DIR}/src/lib)

file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.cpp)

foreach(testSrc ${TEST_SRCS})
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
    target_link_libraries(${testName} ${testDEPS})
  else(testDEPS)
    message("NO TEST DEPS in ${testName}")
  endif(testDEPS)

  add_test(${testName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testName})
endforeach(testSrc)
