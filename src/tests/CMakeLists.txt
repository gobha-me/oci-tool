message("Processing t/src/CMakeList.txt")

file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.cpp)

foreach(testSrc ${TEST_SRCS})
  get_filename_component(testName ${testSrc} NAME_WE)
  set(testName "${testName}_test")

  file(STRINGS ${testSrc} testDEPS REGEX "CMAKE_DEP=.*")

  add_executable(${testName} ${testSrc})

  if(testDEPS)
    string(REPLACE "//CMAKE_DEP=" "" testDEPS ${testDEPS})
    target_link_libraries(${testName} ${testDEPS})
  else(testDEPS)
    message("NO TEST DEPS in ${testName}")
  endif(testDEPS)

  add_test(${testName} ${PROJECT_BINARY_DIR}/../bin/${testName})
endforeach(testSrc)
